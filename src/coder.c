#include "../includes/codexion.h"

void	*coder_routine(void *args)
{
	t_coder	*coder;

	if (args == NULL)
		return (NULL);
	coder = (t_coder *)args;
	wait_for_start(coder->sim);
	return (NULL);
}