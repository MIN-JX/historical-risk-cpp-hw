#!/usr/bin/env python3
"""Regression checks for failures that would invalidate published evidence."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "scripts" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


benchmarks = load("check_benchmark")
documents = load("check_docs")
repository = load("configure_repository")


def complete_measurements():
    rows = []
    for method in ("BM_Sort", "BM_Partition"):
        for count in (100, 1000, 10000, 50000, 100000):
            for confidence in (9500, 9750):
                base = dict(run_name=f"{method}/count:{count}/confidence_bps:{confidence}",
                            repetitions=3, threads=1, iterations=100,
                            real_time=12.0, cpu_time=11.0, time_unit="ns")
                rows.extend(dict(base, run_type="iteration", repetition_index=i)
                            for i in range(3))
                rows.extend(dict(base, run_type="aggregate", aggregate_name=name)
                            for name in ("mean", "median", "stddev", "cv"))
    return {"benchmarks": rows}


class EvidenceChecks(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def benchmark(self, value):
        path = self.root / "measurement.json"
        path.write_text(json.dumps(value), encoding="utf-8")
        return benchmarks.validate(path)

    def test_complete_measurements(self):
        self.assertEqual(self.benchmark(complete_measurements()), (20, 140))

    def test_missing_and_duplicate_repetitions(self):
        for action in ("remove", "duplicate"):
            data = complete_measurements()
            if action == "remove":
                del data["benchmarks"][0]
            else:
                data["benchmarks"][1] = copy.deepcopy(data["benchmarks"][0])
            with self.subTest(action=action), self.assertRaises(ValueError):
                self.benchmark(data)

    def test_failed_nonfinite_and_zero_measurements(self):
        for key, value in (("error_occurred", True), ("cpu_time", float("nan")),
                           ("real_time", 0), ("repetition_index", True)):
            data = complete_measurements()
            data["benchmarks"][0][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.benchmark(data)

    def test_wrong_workload_and_missing_aggregate(self):
        for key, value in (("run_name", "unrelated"), ("aggregate_name", "unexpected")):
            data = complete_measurements()
            data["benchmarks"][3][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.benchmark(data)

    def test_encoded_anchors_and_external_links(self):
        (self.root / "index.html").write_text(
            '<a id="A &amp; B"></a><a href="#A%20%26%20B">valid</a>'
            '<a href="https://example.com/absent">external</a>', encoding="utf-8")
        self.assertEqual(documents.validate(self.root), (1, 1))

    def test_missing_file_anchor_and_path_escape(self):
        for link in ("missing.html", "#missing", "../outside.html"):
            (self.root / "index.html").write_text(f'<a href="{link}">broken</a>', encoding="utf-8")
            with self.subTest(link=link), self.assertRaises(ValueError):
                documents.validate(self.root)

    def test_missing_html_index(self):
        with self.assertRaises(ValueError):
            documents.validate(self.root)

    def test_project_repository_links(self):
        self.assertEqual(repository.links_for_url("https://github.com/example/bond-lab.git"),
                         ("https://github.com/example/bond-lab", "https://example.github.io/bond-lab/"))

    def test_account_pages_repository(self):
        self.assertEqual(repository.links_for_url("https://github.com/example/example.github.io")[1],
                         "https://example.github.io/")

    def test_reject_fake_or_malformed_repository(self):
        for value in ("https://github.com/OWNER/REPOSITORY", "http://github.com/a/b",
                      "https://evil.example/a/b", "https://user@github.com/a/b",
                      "https://github.com/a/b/tree/main", "https://github.com/a/..",
                      "https://github.com/a/b?unexpected=true"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                repository.links_for_url(value)


if __name__ == "__main__":
    unittest.main()
