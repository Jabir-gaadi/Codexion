#include "../includes/codexion.h"

void	*monitor_routine(void *args)
{
	t_sim	*sim;

	if (args == NULL)
		return (NULL);
	sim = (t_sim *)args;
	wait_for_start(sim);
	return (NULL);
}
