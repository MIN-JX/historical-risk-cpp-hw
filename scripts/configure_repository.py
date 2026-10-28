#!/usr/bin/env python3
"""Set the actual repository and Pages links without performing remote actions."""
import argparse
from pathlib import Path
import re
from urllib.parse import urlsplit


def links_for_url(value):
    """Return normalized repository and standard GitHub Pages URLs."""
    parsed = urlsplit(value)
    if (parsed.scheme != "https" or parsed.netloc != "github.com"
            or parsed.query or parsed.fragment):
        raise ValueError("expected https://github.com/OWNER/REPOSITORY")
    parts = parsed.path.strip("/").split("/")
    if len(parts) != 2:
        raise ValueError("expected exactly an owner and a repository")
    owner, repository = parts
    if repository.endswith(".git"):
        repository = repository[:-4]
    if (not re.fullmatch(r"[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?", owner)
            or "--" in owner
            or not re.fullmatch(r"[A-Za-z0-9_.-]{1,100}", repository)
            or repository in (".", "..")):
        raise ValueError("invalid GitHub owner or repository name")
    if owner == "OWNER" or repository == "REPOSITORY":
        raise ValueError("replace OWNER and REPOSITORY with the actual account and repository")
    repo_url = f"https://github.com/{owner}/{repository}"
    pages_base = f"https://{owner.lower()}.github.io/"
    pages_url = (pages_base if repository.lower() == owner.lower() + ".github.io"
                 else pages_base + repository + "/")
    return repo_url, pages_url


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", required=True)
    args = parser.parse_args()
    try:
        repo, pages = links_for_url(args.url)
        path = Path(__file__).resolve().parents[1] / "README.md"
        text = path.read_text(encoding="utf-8")
        start = "<!-- repository-links:start -->"
        end = "<!-- repository-links:end -->"
        if text.count(start) != 1 or text.count(end) != 1:
            raise ValueError("README must contain exactly one repository-link block")
        before, rest = text.split(start)
        _, after = rest.split(end)
        block = (f"\n**Repository:** [source]({repo}) | "
                 f"[Actions]({repo}/actions)\n\n"
                 f"**Documentation:** [Doxygen]({pages}). "
                 "This address requires a successful Pages deployment; "
                 "check the actual workflow before submitting.\n")
        path.write_text(before + start + block + end + after, encoding="utf-8")
        print("Repository:", repo)
        print("Expected Pages URL:", pages)
        print("README updated. No remote changes were made.")
    except (ValueError, OSError) as error:
        parser.exit(2, f"error: {error}\n")


if __name__ == "__main__":
    main()
