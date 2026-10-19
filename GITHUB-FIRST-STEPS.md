# Your first GitHub upload

The prepared source folder is **HauntCue-GitHub**. Copy its CONTENTS into the root of your new repository, so README.md, LICENSE and .gitignore are at the top level. Do not upload the parent Codex folder, work folder or a flash backup.

## First, finish creating the repository

Name it HauntCue, choose a description and visibility, and create it. If GitHub offers a license selector, choose **None**: this project uses the custom LICENSE file supplied here. Do not select MIT or GPL. If you already initialized the repository with a README, .gitignore or license, the prepared files can replace them in your first project update. Review the custom commercial-permission wording before making the project public.

Suggested description: ESP32 Halloween prop controller with web timelines, audio, servo, lighting, reusable props and source-available sharing.

## Easiest ongoing workflow: GitHub Desktop

1. Install GitHub Desktop from https://desktop.github.com/ and sign into your GitHub account.
2. Choose **File → Clone repository** and select the repository you just created. Choose a normal local working folder, such as Documents/GitHub/HauntCue.
3. Open that cloned folder in File Explorer. Copy the CONTENTS of HauntCue-GitHub into it. Preserve its hidden .git folder; do not copy another .git folder into it.
4. In GitHub Desktop, review **Changes**. You should see source, docs, images and tests. You should not see passwords, saved device exports, flash backups, build caches or executable installers.
5. Enter a summary such as **Initial HauntCue source and documentation** and click **Commit to main**. A commit is a saved snapshot on your PC.
6. Click **Push origin**. A push uploads your commits to your repository. Open the repository on GitHub and confirm the README renders and the top-level LICENSE is the custom HauntCue license.
7. For future changes, edit files in that clone, review Changes, commit with a short explanation, then Push origin. Fetch/pull before editing if another computer or collaborator has changed the repository.

If the repository was created under a different default branch name, GitHub Desktop uses that name instead of main. Do not force-push or delete the repository to work around a first-upload error; copy the exact message and ask for help.

## Browser-only alternative

Open your new repository, choose **Add file → Upload files** (or **uploading an existing file** on an empty repository), and drag in the prepared folder’s CONTENTS. Check that README.md is at the root, then commit the upload. Browser uploading does not apply your .gitignore: the curated folder is the important safeguard. Uploading a ZIP file does not unpack the project into the repository.

GitHub Desktop is easier for repeat updates and reviewing .gitignore exclusions. The .gitignore file is not a password scanner; review actual file contents before publishing.

## Add the easy installer as a Release

1. On the repository page, open **Releases → Draft a new release**.
2. Create a tag such as **v1.32.0** on the commit that contains the corresponding firmware source.
3. Use a title such as **HauntCue 1.32.0** and explain the included features, supported/experimental boards and known limitations.
4. Attach the prepared **HauntCue-v1.32-GitHub-Release.zip** as an asset. It contains compiled firmware, installer tool/source materials and documentation; keep it out of ordinary source commits.
5. Review the license, third-party notices and corresponding-source requirements, then publish the release when ready.

The automatically generated Source code ZIP is different from the Windows installer ZIP. Tell friends to download the named installer asset, extract all files and run INSTALL-START-HERE.cmd.

## Three files explained

- **README.md:** the front page explaining what HauntCue does and how to install/use it.
- **LICENSE:** permissions and conditions. This custom version requires written permission before commercial sales of HauntCue-equipped products.
- **.gitignore:** tells Git which untracked files should normally stay out of commits. It does not erase files already uploaded and does not prevent manual browser uploads.

References: GitHub Desktop cloning help: https://docs.github.com/en/desktop/adding-and-cloning-repositories/cloning-and-forking-repositories-from-github-desktop ; GitHub release help: https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository .
