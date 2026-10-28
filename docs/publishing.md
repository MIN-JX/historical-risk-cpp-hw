# Publishing in the project owner's account

The source is ready to transfer as a folder. A public repository and a real
Pages deployment still require the eventual owner's GitHub account. This guide
does not assume or reuse another student's repository.

## Prepare the repository

1. Create an empty public repository on GitHub. Leave automatic README/license
   initialization off when pushing this existing folder.
2. In the source folder, configure the real URL. Replace the two uppercase
   components below with the actual owner and repository name:

```sh
python3 scripts/configure_repository.py --url https://github.com/OWNER/REPOSITORY
```

The script updates the dedicated repository/documentation link block in README.
It prints the expected Pages address; that address becomes usable only after a
successful deployment. It does not authenticate, create a repository, or upload.

3. Run the complete local verification and inspect the changed README.
4. In GitHub repository Settings, choose Pages, then GitHub Actions as the build
   and deployment source. Ensure Actions is enabled for the repository.
5. Push the project. From a newly initialized source folder, one possible command
   sequence is shown below. Replace the URL and use the owner's own Git identity:

```sh
git init -b main
git add .
git commit -m "Add historical risk project with automated verification"
git remote add origin https://github.com/OWNER/REPOSITORY.git
git push -u origin main
```

For a folder already under Git, inspect its current remotes and branch first.
Do not overwrite an existing remote or force-push over unrelated work.

## Check the actual Actions run

The workflow builds on Ubuntu, macOS, and Windows, runs CTest and numerical
oracles, prints benchmark timings, validates JSON, and checks Doxygen links.
Artifacts retain the platform's benchmark JSON and generated HTML. The Pages
job runs after the complete build matrix succeeds, on a push to the default
branch. Pull requests run validation but do not publish.

All platform jobs and the Pages deployment must be green. Read the logs for the
test count and benchmark validation message, download a measurement artifact,
and open the documentation link from README. If a job fails, fix the cause and
push another commit; do not label a failed or unrun workflow as passed.

This workflow uses GitHub's official actions:
[checkout](https://github.com/actions/checkout),
[upload-artifact](https://github.com/actions/upload-artifact),
[upload-pages-artifact](https://github.com/actions/upload-pages-artifact), and
[deploy-pages](https://github.com/actions/deploy-pages).
GitHub's [Pages workflow documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)
describes the required Pages setting and deployment permissions.

## Submit

Keep the source folder, the public repository link, the successful Actions run
link, and the actual documentation link. Record the final commit hash so the
submitted files and public evidence refer to the same version. Local verification
evidence supplied with this folder describes this machine's run, not a later CI
run or another person's authorship.

