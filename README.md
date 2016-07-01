#IW(3) on position

Original code by [Lipovetzky, Ramirez and
Geffner](https://github.com/miquelramirez/ALE-Atari-Width). Code license GPLv2.

Needs the [modified Arcade Learning
Environment](https://github.com/rhaps0dy/ALE-montezuma-modified) to work.

Compile using CMake:

```
$ cmake .
$ make
```

##Contents
Each of the directories under `runs` contains:

- `action_reward_montezuma_iw.1`: list of the actions taken and their rewards.
- `state_montezuma_iw.1`: file with all the states the emulator was in. Can be
	used with the `replay` binary.
- `log.txt`: standard output of the ran search program
- `ale.cfg`: the options used

To re-run a directory, copy the `ale.cfg` file and the `iw` executable to
another directory. Then change `ale.cfg`'s recording directory to the new
directory, and run.

##CMake configuration variables:

- `OUTPUT_EXPLORE`: whether to output the explored positions in `exparr.txt`. Can be displayed with:
	- `plot_explored.py`: in the style of the thesis' Figure 3.3. Works only with
	  the first screen. Can be edited to work with others.
  - `exparr.py`: plots the exploration, without background image, of the whole pyramid
- `PRINT`: whether to print extra information about the search.
- `SHOW_SEARCH`: show, on the screen, the states of the search tree as they are emulated

#Run description
- `3actions`: First run, recalculating every three actions with the latest waiting puzzle algorithm. Shown [here](https://vimeo.com/172891929).
- `3actions-run-2`: Second run, with knowledge of which doors should not be opened. Manages to find the bug shown [here](https://www.youtube.com/watch?v=KSPYzLE0uy8)
- `3actions-seed-1234`: Same as `3actions`, random seed is 1234.
- `3actions-seed-1235`: Same as `3actions`, random seed is 1235.
- `3actions-seed-1236`: Same as `3actions`, random seed is 1236.
- `frameskip10`: Frameskip 10, but recalculate every two actions.
- `no-minimal`: Same as `3actions`, but uses the full action set instead of the restricted set
