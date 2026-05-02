#include "log.h"
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/container.h"

struct cmd_results *cmd_default_title_edge(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "default_title_edge", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	if (strcmp(argv[0], "top") == 0) {
		config->title_edge = WLR_EDGE_TOP;
	} else if (strcmp(argv[0], "bottom") == 0) {
		config->title_edge = WLR_EDGE_BOTTOM;
	} else if (strcmp(argv[0], "left") == 0) {
		config->title_edge = WLR_EDGE_LEFT;
	} else if (strcmp(argv[0], "right") == 0) {
		config->title_edge = WLR_EDGE_RIGHT;
	} else {
		return cmd_results_new(CMD_INVALID,
				"Expected 'default_title_edge <top|bottom|left|right>'");
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
