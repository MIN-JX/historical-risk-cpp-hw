#!/usr/bin/env python3
"""Independent Decimal checks of empirical VaR/ES and the strict CSV CLI."""
import argparse
import csv
from decimal import Decimal, localcontext
import io
import math
from pathlib import Path
import subprocess
import tempfile

METRICS = ("value_at_risk", "expected_shortfall", "mean_loss", "worst_loss")


def oracle(values, confidence):
    with localcontext() as context:
        context.prec = 420
        losses = sorted(Decimal(value) for value in values)
        count = len(losses)
        rank = (count * confidence + 9999) // 10000
        tail_mass = Decimal(count) * (Decimal(1) - Decimal(confidence) / Decimal(10000))
        total = Decimal(0)
        remaining = tail_mass
        for value in reversed(losses):
            mass = min(Decimal(1), remaining)
            total += value * mass
            remaining -= mass
            if remaining == 0:
                break
        return dict(value_at_risk=losses[rank - 1],
                    expected_shortfall=total / tail_mass,
                    mean_loss=sum(losses) / Decimal(count), worst_loss=losses[-1])


def parse_output(text, method, count, confidence):
    rows = list(csv.reader(io.StringIO(text)))
    if not rows or rows[0] != ["metric", "value"]:
        raise AssertionError("missing metric,value header")
    if any(len(row) != 2 for row in rows[1:]):
        raise AssertionError("malformed CSV output")
    values = dict(rows[1:])
    if len(values) != len(rows) - 1:
        raise AssertionError("duplicate output metrics")
    expected_keys = set(METRICS) | {"method", "sample_count", "confidence_bps"}
    if set(values) != expected_keys:
        raise AssertionError(f"unexpected output metrics: {set(values)}")
    if values["method"] != method:
        raise AssertionError("wrong method label")
    if int(values["sample_count"]) != count or int(values["confidence_bps"]) != confidence:
        raise AssertionError("wrong count or confidence")
    for name in METRICS:
        if not math.isfinite(float(values[name])):
            raise AssertionError(f"nonfinite {name}")
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", required=True, type=Path)
    parser.add_argument("--data-dir", type=Path)
    args = parser.parse_args()
    app = str(args.app.resolve())
    comparisons = 0
    subnormal_comparisons = 0
    valid_runs = 0
    invalid_runs = 0

    def run(arguments, *, success=True):
        completed = subprocess.run([app, *arguments], text=True, capture_output=True, timeout=20)
        if success:
            if completed.returncode != 0 or completed.stderr.strip():
                raise AssertionError(f"valid command failed: {arguments}: {completed.stderr}")
        elif completed.returncode == 0 or not completed.stderr.strip() or completed.stdout.strip():
            raise AssertionError(f"invalid command accepted or misreported: {arguments}")
        return completed.stdout

    def check(path, source, confidence, method):
        nonlocal comparisons, valid_runs
        output = run(["--input", str(path), "--confidence-bps", str(confidence), "--method", method])
        actual = parse_output(output, method, len(source), confidence)
        expected = oracle(source, confidence)
        with localcontext() as context:
            context.prec = 420
            for name, reference in expected.items():
                measured = Decimal(actual[name])
                tolerance = max(abs(reference), Decimal("1e-300")) * Decimal("2e-11")
                if abs(measured - reference) > tolerance:
                    raise AssertionError(f"{name} mismatch: {measured} vs {reference}")
                comparisons += 1
        valid_runs += 1

    datasets = [
        ["-50", "-20", "0", "10", "20", "30", "40", "60", "80", "100"],
        ["-20", "0", "10", "30", "40"],
        ["0", "10", "10", "10", "100"],
        ["-9", "-5", "-2"],
        ["17", "17", "17", "17"],
        ["-13"],
        ["0", "0", "0"],
        ["-0.75", "0.25", "0.5", "1.5"],
        ["1e300", "1", "-1e300"],
        ["1e-200", "2e-200", "3e-200"],
    ]
    with tempfile.TemporaryDirectory(prefix="risk-cli-check-") as directory:
        root = Path(directory)
        for index, source in enumerate(datasets):
            path = root / f"sample-{index}.csv"
            path.write_text("loss\n" + "\n".join(source) + "\n", encoding="utf-8")
            for confidence in (1, 5000, 8000, 8250, 9750, 9999):
                for method in ("sort", "partition"):
                    check(path, source, confidence, method)
        for index, content in enumerate((b"\xef\xbb\xbfloss\r\n -1 \r\n 2\t\r\n 3\r\n",
                                         b" loss \n\t-1\t\n 2 \n3\n")):
            path = root / f"portable-{index}.csv"
            path.write_bytes(content)
            for method in ("sort", "partition"):
                check(path, ["-1", "2", "3"], 8000, method)
        # Do not use an absolute tolerance here: it would hide silent rounding
        # of the smallest representable positive double to zero.
        subnormal = root / "subnormal.csv"
        subnormal.write_text("loss\n" + "4.9406564584124654e-324\n" * 10, encoding="utf-8")
        smallest = float.fromhex("0x0.0000000000001p-1022")
        for method in ("sort", "partition"):
            actual = parse_output(run(["--input", str(subnormal), "--confidence-bps", "8000",
                                       "--method", method]), method, 10, 8000)
            for name in METRICS:
                if float(actual[name]) != smallest:
                    raise AssertionError(f"smallest positive double was lost in {name}")
                subnormal_comparisons += 1
            valid_runs += 1
        good = root / "good.csv"
        good.write_text("loss\n1\n2\n3\n", encoding="utf-8")
        default = parse_output(run(["--input", str(good)]), "partition", 3, 9500)
        if Decimal(default["value_at_risk"]) != 3 or Decimal(default["expected_shortfall"]) != 3:
            raise AssertionError("incorrect default confidence")
        bad_commands = [
            [], ["--input"], ["--unknown", "1"], ["--input", str(root / "absent.csv")],
            ["--input", str(good), "--input", str(good)],
            ["--input", str(good), "--confidence-bps", "0"],
            ["--input", str(good), "--confidence-bps", "10000"],
            ["--input", str(good), "--confidence-bps", "-1"],
            ["--input", str(good), "--confidence-bps", "95.0"],
            ["--input", str(good), "--confidence-bps", "9500tail"],
            ["--input", str(good), "--confidence-bps", "999999999999999999999"],
            ["--input", str(good), "--confidence-bps", "9500", "--confidence-bps", "9500"],
            ["--input", str(good), "--method", "unknown"],
            ["--input", str(good), "--method", "sort", "--method", "partition"],
            ["--help", "--input", str(good)],
        ]
        for command in bad_commands:
            run(command, success=False)
            invalid_runs += 1
        invalid_csv = ["", "loss\n", "pnl\n1\n", '"loss"\n1\n', "\nloss\n1\n",
                       "loss\n1\n\n2\n", "loss\n1,2\n", 'loss\n"1"\n',
                       "loss\nNaN\n", "loss\ninf\n", "loss\n-inf\n",
                       "loss\n1x\n", "loss\n1e9999\n", "loss\n   \n", "loss\n1e-999\n"]
        for index, content in enumerate(invalid_csv):
            path = root / f"invalid-{index}.csv"
            path.write_text(content, encoding="utf-8")
            run(["--input", str(path)], success=False)
            invalid_runs += 1
        too_many = root / "too-many.csv"
        too_many.write_text("loss\n" + "1\n" * 1000001, encoding="utf-8")
        run(["--input", str(too_many)], success=False)
        invalid_runs += 1
        if args.data_dir:
            fixture = args.data_dir / "example_losses.csv"
            with fixture.open(encoding="utf-8", newline="") as stream:
                rows = list(csv.reader(stream))
            if rows[0] != ["loss"]:
                raise AssertionError("example CSV header changed")
            source = [row[0] for row in rows[1:]]
            for method in ("sort", "partition"):
                check(fixture, source, 8000, method)
    help_text = run(["--help"])
    if "--input" not in help_text or "--confidence-bps" not in help_text:
        raise AssertionError("help does not describe required arguments")
    print(f"CLI validation passed: {comparisons} Decimal metric comparisons, "
          f"{subnormal_comparisons} exact subnormal checks across {valid_runs} oracle runs, "
          f"{invalid_runs} invalid inputs, defaults and help.")


if __name__ == "__main__":
    main()
