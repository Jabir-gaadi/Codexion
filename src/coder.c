#include "../includes/codexion.h"

// void	*coder_routine(void *args)
// {
// 	t_coder	*coder;

// 	if (args == NULL)
// 		return (NULL);
// 	coder = (t_coder *)args;
// 	wait_for_start(coder->sim);
// 	while (sim_is_running(coder->sim))
// 	{
// 		take_dongles_basic(coder);
// 		log_action(coder->sim, coder->id, "is compiling");
// 		sim_sleep(coder->sim, coder->sim->config.time_to_compile);
// 		release_dongles_basic(coder);
// 		coder->compiles_done++;
// 		log_action(coder->sim, coder->id, "is debugging");
// 		sim_sleep(coder->sim, coder->sim->config.time_to_debug);
// 		log_action(coder->sim, coder->id, "is refactoring");
// 		sim_sleep(coder->sim, coder->sim->config.time_to_refactor);
// 	}
// 	return (NULL);
// }

void	*coder_routine(void *args)
{
	t_coder		*coder;
	long long	current_time;
	long long	deadline;
	int			compile_completed;

	if (args == NULL)
		return (NULL);
	coder = (t_coder *)args;
	if (coder->sim == NULL)
		return (NULL);
	wait_for_start(coder->sim);
	while (sim_is_running(coder->sim))
	{
		if (take_dongles_basic(coder) == 0)
			return (NULL);
		if (get_time_in_ms(&current_time) == 0
			|| add_time_ms(current_time,
				coder->sim->config.time_to_burnout,
				&deadline) == 0)
		{
			release_dongles_basic(coder);
			return (NULL);
		}
		pthread_mutex_lock(&coder->sim->state_mutex);
		if (coder->sim->stop_reason != STOP_NONE)
		{
			pthread_mutex_unlock(&coder->sim->state_mutex);
			release_dongles_basic(coder);
			return (NULL);
		}
		coder->start_of_lastcompile = current_time;
		coder->deadline = deadline;
		pthread_mutex_unlock(&coder->sim->state_mutex);
		if (coder->compiles_done == coder->sim->config.number_of_compiles_required)
			return (NULL);
		log_action(coder->sim, coder->id, "is compiling");
		compile_completed = sim_sleep(coder->sim,
				coder->sim->config.time_to_compile);
		if (release_dongles_basic(coder) == 0)
			return (NULL);
		if (compile_completed == 0)
			return (NULL);
		pthread_mutex_lock(&coder->sim->state_mutex);
		coder->compiles_done++;
		if (coder->compiles_done >= coder->sim->config.number_of_compiles_required)
			coder->finished = 1;
		pthread_mutex_unlock(&coder->sim->state_mutex);
		if (coder->finished)
			return (NULL);
		if (!sim_is_running(coder->sim))
			return (NULL);
		log_action(coder->sim, coder->id, "is debugging");
		if (sim_sleep(coder->sim,
				coder->sim->config.time_to_debug) == 0)
			return (NULL);
		if (!sim_is_running(coder->sim))
			return (NULL);
		log_action(coder->sim, coder->id, "is refactoring");
		if (sim_sleep(coder->sim,
				coder->sim->config.time_to_refactor) == 0)
			return (NULL);
	}
	return (NULL);
}