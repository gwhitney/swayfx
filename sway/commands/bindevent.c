#include "sway/config.h"

#include "log.h"
#include "sway/commands.h"
#include "sway/criteria.h"
#include "sway/desktop/transaction.h"

void free_event_binding(struct sway_event_binding *binding) {
	if (!binding) {
		return;
	}
	if (binding->criteria) {
		criteria_destroy(binding->criteria);
	}
	free(binding->event);
	free(binding->command);
	free(binding);
}

/**
 * Add event binding to config
 */
static char *msg
	= "Overwriting unconditional binding for event '%s' to `%s` from `%s`";
static struct cmd_results *event_binding_add(
	struct sway_event_binding *binding, bool warn
) {
	list_t *mode_bindings = config->current_mode->event_bindings;
	bool must_add = true;
	if (!(binding->criteria)) {
		// overwrite any unconditional binding for the same event
		for (int i = 0; i < mode_bindings->length; ++i) {
			struct sway_event_binding *b = mode_bindings->items[i];
			if (strcmp(b->event, binding->event) == 0
				&& !(b->criteria)
			) {
				sway_log(
					SWAY_INFO,
					"Overwriting unconditional binding for "
						"event '%s' to `%s` from `%s`",
					b->event, binding->command, b->command);
				if (warn) {
					config_add_swaynag_warning(msg, b->event,
						binding->command, b->command);
				}
				free_event_binding(b);
				mode_bindings->items[i] = binding;
				must_add = false;
			}
		}
	}

	if (must_add) {
		list_add(mode_bindings, binding);
		sway_log(SWAY_DEBUG, "bindevent - '%s' to `%s`",
			binding->event, binding->command);
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}

/**
 * Unbinds an event entirely
 */
struct cmd_results *cmd_unbindevent(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "unbindevent", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}
	const char *event = argv[0];
	list_t *mode_bindings = config->current_mode->event_bindings;
	bool found = false;
	for (int i = 0; i < mode_bindings->length; ++i) {
		struct sway_event_binding *b = mode_bindings->items[i];
		if (strcmp(b->event, event) == 0) {
			found = true;
			sway_log(SWAY_DEBUG,
				"unbindevent - removed command `%s` from '%s'",
				b->command, event);
			free_event_binding(b);
			list_del(mode_bindings, i);
		}
	}

	if (found) {
		return cmd_results_new(CMD_SUCCESS, NULL);
	}
	return cmd_results_new(CMD_FAILURE, "No binding for event '%s'", event);
}

struct cmd_results *cmd_bindevent(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "bindevent", EXPECTED_AT_LEAST, 2))) {
		return error;
	}

	bool warn = strcmp("--no-warn", argv[0]) != 0;
	if (!warn) {
		++argv;
		--argc;
	}

	struct criteria *criteria = NULL;
	if (argv[0][0] == '[') {
		char *e = NULL;
		criteria = criteria_parse(argv[0], &e);
		if (!criteria) {
			struct cmd_results *error
				= cmd_results_new(CMD_INVALID, "%s", e);
			free(e);
			return error;
		}
		++argv;
		--argc;
	}
	if (argc < 2) {
		return cmd_results_new(CMD_INVALID,
			"bindevent requires both event and command");
	}
	struct sway_event_binding *binding
		= calloc(1, sizeof(struct sway_event_binding));
	binding->criteria = criteria;
	binding->event = strdup(argv[0]);
	binding->command = join_args(++argv, --argc);
	return event_binding_add(binding, warn);
}

void binding_events_execute(const char* event, struct sway_node *node) {
	bool criteria_matched = false;
	bool event_matched = false;
	list_t *bindings = config->current_mode->event_bindings;
	for (int i = 0; i < bindings->length; ++i) {
		struct sway_event_binding *binding = bindings->items[i];
		if (strcmp(event, binding->event) != 0) {
			continue;
		}
		if (!(binding->criteria)) {
			event_matched = true;
			continue;
		}
		// Here we have a criterion. Right now it appears that criteria
		// can only match containers. So if this is not a container node,
		// we can move on:
		if (node->type != N_CONTAINER) {
			continue;
		}
		if (!criteria_matches_container_seat(binding->criteria,
			node->sway_container, input_manager_current_seat())
		) {
			continue;
		}
		// "Perfect" match, so we can execute and continue
		criteria_matched = true;
		list_t *res_list = execute_command(binding->command,
			input_manager_current_seat(), node);
		for (int r = 0; r < res_list->length; ++r) {
			struct cmd_results *results = res_list->items[r];
			if (results->status != CMD_SUCCESS) {
				sway_log(SWAY_DEBUG,
					"error in command for binding: %s (%s)",
					binding->command, results->error);
			}
			free_cmd_results(results);
		}
		list_free(res_list);
	}
	if (criteria_matched || !event_matched) {
		transaction_commit_dirty();
		return;
	}
	// There was an unconditional match and no match with a criterion,
	// so run the (first) unconditional match;
	for (int i = 0; i < bindings->length; ++i) {
		struct sway_event_binding *binding = bindings->items[i];
		if (strcmp(event, binding->event) != 0 || binding->criteria) {
			continue;
		}
		list_t *res_list = execute_command(binding->command,
			input_manager_current_seat(), node);
		for (int r = 0; r < res_list->length; ++r) {
			struct cmd_results *results = res_list->items[r];
			if (results->status != CMD_SUCCESS) {
				sway_log(SWAY_DEBUG,
					"error in command for binding: %s (%s)",
					binding->command, results->error);
			}
			free_cmd_results(results);
		}
		list_free(res_list);
		break;
	}
	transaction_commit_dirty();
}
