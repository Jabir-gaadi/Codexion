/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/18 15:28:58 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/18 15:28:59 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int	main(int ac, char **av)
{
	t_config	config;
	t_sim		sim;
	int			is_joined;

	if (parse_args(ac, av, &config) == 0)
		return (1);
	if (sim_init(&sim, &config) == 0)
		return (2);
	if (sim_start_threads(&sim) == 0)
	{
		sim_destroy(&sim);
		return (3);
	}
	is_joined = sim_join_threads(&sim);
	sim_destroy(&sim);
	if (is_joined == 0)
		return (4);
	return (0);
}
