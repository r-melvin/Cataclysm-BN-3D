# Taking upstream changes

This fork takes game-logic changes from `cataclysmbn/Cataclysm-BN` and keeps its own frontend
(3D renderer, onboarding, status report, UI theme). Upstream renderer work is not taken.

`tools/upstream_sync/renderer_paths.txt` lists the paths the fork owns, one git pathspec per line.
Edit it when the fork starts owning another file.

```sh
git fetch upstream
tools/upstream_sync/check.sh report      # classify each upstream commit
git merge upstream/main                  # on a sync branch, never on main directly
tools/upstream_sync/check.sh guard       # fails if the merge changed a frontend path
```

- `report` marks each commit `logic` (touches no frontend path) or `FRONTEND` (lists the paths), then
  lists files that both upstream and the fork changed, which are the likely conflicts.
- `guard` compares the merge commit with its first parent. A frontend path that differs means the
  merge took upstream renderer work; restore it with `git checkout HEAD^1 -- <path>` and amend the merge.
- Build `cataclysm-bn-tiles` and `cata_test-tiles` and run `[r3d]` before pushing the sync branch.

The guard catches paths, not intent: an upstream logic change that needs a matching frontend change
(a new field a renderer reads, say) still has to be found by reading the report.
