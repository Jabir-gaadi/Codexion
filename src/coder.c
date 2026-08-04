#include "../includes/codexion.h"

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
	if (coder->id % 2 == 0)
		usleep(500);
	while (sim_is_running(coder->sim))
	{
		if (coder->finished == 1)
			return (NULL);
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
		log_action(coder->sim, coder->id, "is compiling");
		compile_completed = sim_sleep(coder->sim,
				coder->sim->config.time_to_compile);
		if (release_dongles_basic(coder) == 0)
			return (NULL);
		if (compile_completed == 0)
			return (NULL);
		// if (finished && !sim_is_running(coder->sim))
		// 	return (NULL);
		// if (!sim_is_running(coder->sim))
		// 	return (NULL);
		log_action(coder->sim, coder->id, "is debugging");
		if (sim_sleep(coder->sim,
				coder->sim->config.time_to_debug) == 0)
			return (NULL);
		// if (finished && !sim_is_running(coder->sim))
		// 	return (NULL);
		// if (!sim_is_running(coder->sim))
		// 	return (NULL);
		log_action(coder->sim, coder->id, "is refactoring");
		if (sim_sleep(coder->sim,
				coder->sim->config.time_to_refactor) == 0)
			return (NULL);
		pthread_mutex_lock(&coder->sim->state_mutex);
		coder->compiles_done++;
		if (coder->compiles_done == coder->sim->config.number_of_compiles_required)
			coder->finished = 1;
		pthread_cond_signal(&coder->sim->monitor_cond);
		pthread_mutex_unlock(&coder->sim->state_mutex);
		if (coder->finished || !sim_is_running(coder->sim))
			return (NULL);
	}
	return (NULL);
}