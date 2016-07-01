#IW(3) on position

Original code by Lipovetzky, Ramirez and Geffner. Code license GPLv2.

Needs the modified Arcade Learning Environment and `cmake` to work.

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
