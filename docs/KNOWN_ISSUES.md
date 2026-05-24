# Known Issues

- Search/replace project scan covers up to 5000 files (raised from 500 in v0.1.2-beta). The per-file 1 MB ceiling is unchanged. Repos with more than 5000 indexable files still see truncation; streaming search is deferred to V2.
