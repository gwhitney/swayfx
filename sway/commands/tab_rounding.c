#include "sway/commands.h"
#include "sway/config.h"
#include "sway/output.h"
#include "sway/tree/container.h"
#include "sway/tree/root.h"

static void arrange_title_bar_iterator(struct sway_container *con, void *data) {
	container_arrange_title_bar(con);
}

struct cmd_results *cmd_tab_rounding(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "tab_rounding", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	if (strcmp(argv[0], "corner") == 0) {
		config->tab_rounding = TAB_CORNER;
	} else if (strcmp(argv[0], "all") == 0) {
		config->tab_rounding = TAB_ALL;
	} else if (strcmp(argv[0], "none") == 0) {
		config->tab_rounding = TAB_NONE;
	} else {
		return cmd_results_new(CMD_INVALID,
				"Expected 'tab_rounding corner|all|none'");
	}

	root_for_each_container(arrange_title_bar_iterator, NULL);

	return cmd_results_new(CMD_SUCCESS, NULL);
}
