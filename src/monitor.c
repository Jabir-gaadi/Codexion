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
		pthread_mutex_lock(&sim->state_mutex);
		if (sim->stop_reason != STOP_NONE)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			return (NULL);
		}
		get_time_in_ms(&current_time);
		all_compile = 1;
		idx = 0;
		while (idx < sim->config.number_of_coders)
		{
			if (sim->coders[idx].compiles_done < sim->config.number_of_compiles_required)
				all_compile = 0;
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
			sim_request_stop(sim, STOP_COMPLETED, sim->coders[idx].id, current_time - sim->start_time);
			return (NULL);
		}
		pthread_mutex_unlock(&sim->state_mutex);
		usleep(1000);
	}
}








































//  void	*monitor_routine(void *args)
//  {
// 	t_sim		*sim;
// 	long long	current_time;
// 	long long	timestamp;
// 	int			burned_id;
// 	int			all_completed;
// 	int			i;

// 	if (args == NULL)
// 		return (NULL);
// 	sim = (t_sim *)args;
// 	wait_for_start(sim);
// 	while (1)
// 	{
// 		if (get_time_in_ms(&current_time) == 0)
// 		{
// 			pthread_mutex_lock(&sim->state_mutex);
// 			sim->stop_reason = STOP_ERROR;
// 			pthread_mutex_unlock(&sim->state_mutex);
// 			return (NULL);
// 		}
// 		burned_id = 0;
// 		all_completed = 1;
// 		pthread_mutex_lock(&sim->state_mutex);
// 		if (sim->stop_reason != STOP_NONE)
// 		{
// 			pthread_mutex_unlock(&sim->state_mutex);
// 			return (NULL);
// 		}
// 		i = 0;
// 		while (i < sim->config.number_of_coders)
// 		{
// 			if (current_time >= sim->coders[i].deadline)
// 			{
// 				burned_id = sim->coders[i].id;
// 				sim->stop_reason = STOP_BURNOUT;
// 				break ;
// 			}
// 			if (sim->coders[i].compiles_done
// 				< sim->config.number_of_compiles_required)
// 				all_completed = 0;
// 			i++;
// 		}
// 		if (burned_id == 0 && all_completed)
// 			sim->stop_reason = STOP_COMPLETED;
// 		pthread_mutex_unlock(&sim->state_mutex);
// 		if (burned_id != 0)
// 		{
// 			timestamp = current_time - sim->start_time;
// 			pthread_mutex_lock(&sim->log_mutex);
// 			printf("%lld %d burned out\n", timestamp, burned_id);
// 			pthread_mutex_unlock(&sim->log_mutex);
// 			return (NULL);
// 		}
// 		if (all_completed)
// 			return (NULL);
// 		usleep(1000);
// 	}
//  }