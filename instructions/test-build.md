**QDOS/386 — Current Directory Build Verification Only**

I have manually reconciled the public repository layout after discovering significant source-tree drift, including duplicate kernel source directories.

Treat the **current working directory and its current contents** as the authoritative candidate. Do not restore an earlier export, reorganize the tree, move files or start another source-consolidation project.

Your assignment is strictly to verify the current checkout.

1. Record the current directory, Git HEAD, branch, working-tree status and source-tree layout before changing anything.
2. Identify the actual production kernel source tree and Makefile dependency graph. Report any duplicate kernel trees, but do not modify or delete them.
3. Attempt to build the current project using its existing Makefiles and available historical toolchain.
4. Do not silently substitute files from the private development repository or accepted release archives.
5. Report missing dependencies and build failures with their actual causes.
6. If the build succeeds, verify the generated kernel components against the accepted R6A hashes wherever the outputs are directly comparable.
7. Do not claim the complete single-disk image is reproducible unless the required inputs are present and the exact accepted image hash is obtained.
8. Check that the SDK and application source directories remain present, but do not modify them.
9. Produce a concise report of what builds, what fails, what is duplicated and which dependencies remain external.

**Do not run `git clean`, `git reset --hard`, force checkout, merge, push or publish.**

Do not edit source, Makefiles or `.gitignore` during this verification pass.

Do not repeat the entire QEMU acceptance matrix.

Stop after reporting the results. I will decide what changes are authorized next.
