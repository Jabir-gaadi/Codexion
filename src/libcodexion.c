#include "../includes/codexion.h"

void	join_coders_threads(t_sim *sim, int total_thread)
{
	int	idx;

	idx = 0;
	while (idx < total_thread)
	{
		pthread_join(sim->coders[idx].thread, NULL);
		idx++;
	}
}