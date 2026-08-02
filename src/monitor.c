#include "../includes/codexion.h"

void	*monitor_routine(void *args)
{
	t_sim		*sim;
	long long	current_time;
	int			all_compile;
	int			idx;

	if (args == NULL)
		return (NULL);
	sim = (t_sim *)args;
	wait_for_start(sim);
	while (1)
	{
		if (get_time_in_ms(&current_time) == 0)
			sim_request_stop(sim, STOP_ERROR, 0, 0);
		pthread_mutex_lock(&sim->state_mutex);
		if (sim->stop_reason != STOP_NONE)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			return (NULL);
		}
		all_compile = 1;
		idx = 0;
		while (idx < sim->config.number_of_coders)
		{
			if (sim->coders[idx].compiles_done < sim->config.number_of_compiles_required)
				all_compile = 0;
			if (sim->coders[idx].compiles_done == sim->config.number_of_compiles_required)
			{
				idx++;
				continue;
			}
			if (current_time >= sim->coders[idx].deadline)
			{
				pthread_mutex_unlock(&sim->state_mutex);
				sim_request_stop(sim, STOP_BURNOUT, sim->coders[idx].id, current_time - sim->start_time);
				return (NULL);
			}
			idx++;
		}
		if (all_compile)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			sim_request_stop(sim, STOP_COMPLETED, 0, current_time - sim->start_time);
			return (NULL);
		}
		pthread_mutex_unlock(&sim->state_mutex);
		usleep(1000);
	}
	return (NULL);
}
