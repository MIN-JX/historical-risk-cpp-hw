#!/usr/bin/env python3
"""Validate Doxygen HTML references to local files and fragment anchors."""
import argparse
import html
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


class References(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.anchors = set()
        self.links = []

    def handle_starttag(self, tag, attributes):
        values = dict(attributes)
        for key in ("id", "name"):
            if values.get(key):
                self.anchors.add(values[key])
        for key in ("href", "src"):
            if values.get(key):
                self.links.append(values[key])

    handle_startendtag = handle_starttag


def validate(directory):
    root = Path(directory).resolve()
    pages = sorted(root.rglob("*.html"))
    if not pages or not (root / "index.html").is_file():
        raise ValueError("no Doxygen HTML index found")
    parsed = {}
    for page in pages:
        document = References()
        document.feed(page.read_text(encoding="utf-8"))
        parsed[page.resolve()] = document
    failures = []
    count = 0
    for page, document in parsed.items():
        for raw in document.links:
            link = urlsplit(html.unescape(raw))
            if link.scheme or link.netloc:
                continue
            count += 1
            target = (page.parent / unquote(link.path)).resolve() if link.path else page
            if not target.is_relative_to(root):
                failures.append(f"{page.name}: reference leaves documentation: {raw}")
            elif not target.is_file():
                failures.append(f"{page.name}: missing file: {raw}")
            elif link.fragment and target.suffix.lower() == ".html":
                target_page = parsed.get(target)
                if target_page is None or unquote(link.fragment) not in target_page.anchors:
                    failures.append(f"{page.name}: missing anchor: {raw}")
    if failures:
        raise ValueError("\n".join(failures[:30]))
    return len(pages), count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("html_directory", type=Path)
    arguments = parser.parse_args()
    try:
        pages, references = validate(arguments.html_directory)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Documentation validation failed: {error}\n")
    print(f"Documentation validation passed: {pages} HTML pages, "
          f"{references} local references.")


if __name__ == "__main__":
    main()
