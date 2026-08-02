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

static int	print_parse_error(int error)
{
	if (error == 2 || error == 0)
	{
		if (error == 2)
			printf("From arg2-arg8 should a valid number and\n");
		printf("Command to run should exactly like that:\n");
		printf("./codexion 2 100 20 20 20 1 10 fifo (OR edf)");
		return (2);
	}
	else if (error == 3)
	{
		printf("For scheduler program accept just fifo OR edf\n");
		return (3);
	}
	return (1);
}

int	main(int ac, char **av)
{
	t_config	config;
	t_sim		sim;
	int			is_valid_parse;

	is_valid_parse = parse_args(ac, av, &config);
	if (is_valid_parse != 1)
		return (print_parse_error(is_valid_parse));
	if (sim_init(&sim, &config) == 0)
	{
		printf("The simulation failed...");
		return (4);
	}
	if (sim_start_threads(&sim) == 0)
	{
		printf("Problem in the installations of threads...");
		sim_destroy(&sim);
		return (3);
	}
	sim_join_threads(&sim);
	sim_destroy(&sim);
	return (0);
}

// 1. subject.txt
// 2. codexion.h
// 3. main.c
// 4. parsing.c
// 5. init.c
// 6. time_utils.c
// 7. simulation.c
// 8. heap.c
// 9. dongle.c
// 10. acquire.c
// 11. coder.c
// 12. monitor.c
// 13. logging.c