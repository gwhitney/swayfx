#include <strings.h>
#include "log.h"
#include "sway/commands.h"
#include "sway/tree/arrange.h"
#include "sway/tree/workspace.h"

static enum wlr_edges parse_edge_string(char* s) {
	if (strcasecmp(s, "top") == 0) {
		return WLR_EDGE_TOP;
	} else if (strcasecmp(s, "bottom") == 0) {
		return WLR_EDGE_BOTTOM;
	} else if (strcasecmp(s, "left") == 0) {
		return WLR_EDGE_LEFT;
	} else if (strcasecmp(s, "right") == 0) {
		return WLR_EDGE_RIGHT;
	}
	return WLR_EDGE_NONE;
}

struct cmd_results *cmd_title_edge(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "title_edge", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	struct sway_container *container = config->handler_context.container;
	if (!container) {
		return cmd_results_new(CMD_INVALID, "No container for title_edge");
	}

	enum wlr_edges new_edge = parse_edge_string(argv[0]);
	if (new_edge == WLR_EDGE_NONE) {
		return cmd_results_new(CMD_INVALID,
			"Expected title_edge top|bottom|left|right");
	}

	enum wlr_edges old_edge = container->pending.title_edge;
	if (new_edge != old_edge) {
		container->pending.title_edge = new_edge;
		// now have to update the container's _parent_
		struct sway_container *parent = container->pending.parent;
		if (parent) {
			arrange_container(parent);
		} else {
			arrange_workspace(config->handler_context.workspace);
		}
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
