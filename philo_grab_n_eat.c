/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   philo_grab_n_eat.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: daejlee <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/10/25 19:41:38 by daejlee           #+#    #+#             */
/*   Updated: 2022/10/25 19:41:39 by daejlee          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "philosophers.h"
#include <stdio.h>
#include <unistd.h>

void	*kill_single_philo(t_philo_profile *p, struct timeval *time)
{
	__int64_t		temp;

	usleep(p->die_time * 1000);
	gettimeofday(time, NULL);
	temp = time->tv_sec * 1000 + time->tv_usec / 1000 - p->time_init_val;
	printf("%llu 1 died\n", temp);
	return (NULL);
}

int	is_fork_available(t_philo_profile *p)
{
	pthread_mutex_lock(p->m_fork_stat);
	if (*p->fork_stat[0] && *p->fork_stat[1])
		return (0);
	return (1);
}

int	unlock_fork(t_philo_profile *p)
{
	pthread_mutex_lock(p->m_fork_stat);
	*p->fork_stat[0] = 1;
	*p->fork_stat[1] = 1;
	pthread_mutex_unlock(p->m_fork_stat);
	pthread_mutex_unlock(p->m_fork_slot[0]);
	pthread_mutex_unlock(p->m_fork_slot[1]);
	return (1);
}

int	is_flags_all_up(int *must_eat_flags, int philo_num)
{
	int	i;
	
	i = 0;
	while (i < philo_num)
	{
		if (!must_eat_flags[i])
			return (1);
		i++;
	}
	return (0);
}

int	is_termination(t_philo_profile *p)
{
	__int64_t		temp;
	struct timeval	time_now;

	pthread_mutex_lock(p->m_t_flag_adr);
	if (*(p->t_flag_adr))
	{
		pthread_mutex_unlock(p->m_t_flag_adr);
		return (0);
	}
	
	if (p->must_eat_flag)
	{
		pthread_mutex_lock(p->m_must_eat_flag);
		if (!is_flags_all_up(p->manager_adr->must_eat_flags, p->manager_adr->philo_num))
		{
			*(p->t_flag_adr) = 1;
			pthread_mutex_unlock(p->m_t_flag_adr);
			pthread_mutex_unlock(p->m_must_eat_flag);
			return (0);
		}
		pthread_mutex_unlock(p->m_must_eat_flag);
	}

	gettimeofday(&time_now, NULL);
	temp = (time_now.tv_sec - p->r_eat.tv_sec) * 1000 + (time_now.tv_usec - p->r_eat.tv_usec) / 1000;
	if (temp >= p->die_time)
	{
		*(p->t_flag_adr) = 1;
		temp = time_now.tv_sec * 1000 + time_now.tv_usec / 1000 - p->time_init_val;
		printf("%llu %i died\n", temp, p->idx);
		pthread_mutex_unlock(p->m_t_flag_adr);
		return (0);
	}
	return (1);
}

static int	gne_sleep(t_philo_profile *p, struct timeval *time)
{
	__int64_t	temp;
	int			think_time;

	pthread_mutex_lock(p->m_time_adr);
	gettimeofday(time, NULL);
	p->r_sleep = *time;
	temp = p->r_sleep.tv_sec * 1000 + p->r_sleep.tv_usec / 1000 - p->time_init_val;
	pthread_mutex_unlock(p->m_time_adr);
	if (!is_termination(p))
		return (1);
	printf("%llu %i is sleeping\n", temp, p->idx);
	pthread_mutex_unlock(p->m_t_flag_adr);
	if (p->eat_time + p->sleep_time > p->die_time)
	{
		usleep((p->die_time - p->eat_time) * 1000);
		return (1);
	}
	usleep(p->sleep_time * 1000);
	pthread_mutex_lock(p->m_time_adr);
	gettimeofday(time, NULL);
	p->r_think = *time;
	temp = p->r_think.tv_sec * 1000 + p->r_think.tv_usec / 1000 - p->time_init_val;
	pthread_mutex_unlock(p->m_time_adr);
	if (!is_termination(p))
		return (1);
	printf("%llu %i is thinking\n", temp, p->idx);
	pthread_mutex_unlock(p->m_t_flag_adr);
	if (p->manager_adr->philo_num % 2)
		think_time = (p->eat_time * 2 - p->sleep_time) * 1000;
	else
		think_time = 100;
	usleep(think_time);
	return (0);
}

int	grab_eat_sleep(t_philo_profile *p, struct timeval *time)
{
	__uint64_t		temp;
	if (!is_termination(p))
		return (unlock_fork(p));
	pthread_mutex_unlock(p->m_t_flag_adr);
	pthread_mutex_lock(p->m_time_adr);
	gettimeofday(time, NULL);
	p->r_eat = *time;
	temp = p->r_eat.tv_sec * 1000 + p->r_eat.tv_usec / 1000 - p->time_init_val;
	pthread_mutex_unlock(p->m_time_adr);

	if (!is_termination(p))
		return (unlock_fork(p));
	printf("%llu %i is eating\n", temp, p->idx);
	p->eat_cnt++;
	if (p->eat_cnt == p->must_eat)
	{
		pthread_mutex_lock(p->m_must_eat_flag);
		*p->must_eat_flag = 1;
		pthread_mutex_unlock(p->m_must_eat_flag);
	}
	pthread_mutex_unlock(p->m_t_flag_adr);

	if (p->eat_time >= p->die_time)
	{
		usleep(p->die_time * 1000);
		return (unlock_fork(p));
	}
	usleep(p->eat_time * 1000);
	unlock_fork(p);
	return (gne_sleep(p, time));
}

/*
must_eat int arr -> use of flag slots.
check whole slots in is_termination.
*/

void	*routine(void *philo_info)
{
	t_philo_profile	*p;
	struct timeval	*time;
	__int64_t		temp;

	p = (t_philo_profile *)philo_info;
	time = p->time_adr;
	pthread_mutex_lock(p->m_time_adr);
	gettimeofday(time, NULL);
	p->r_eat = *time;
	pthread_mutex_unlock(p->m_time_adr);

	if (!(p->m_fork_slot[1]))
		return (kill_single_philo(p, time));

	if (p->manager_adr->philo_num % 2)
	{
		if (p->idx == p->manager_adr->philo_num)
			usleep(p->eat_time * 2 * 1000);
		else if (p->idx % 2)
			usleep(1000);
	}
	else if (p->idx % 2) //홀수 다 재워~~
		usleep(1000);

//	./philo 5 800 200 300일 때
//	A	eat[0~200]		sleep[200~500]		think[500~600]		eat[600~800]		sleep[800~1100]
//	B	think[0~200]	eat[200~400]		sleep[400~700]		think[700~800]		eat[800~1000]
//	C	think[0~200]	think[200~400]		eat[400~600]		sleep[600~900]		think[900~1000]
//	./philo 5 800 300 200일 때
//	A	eat[0~300]		sleep[300~500]		think[500~900]		eat[600~800]		sleep[800~1100]
//	B	think[0~300]	eat[300~600]		sleep[600~800]		think[700~800]		eat[800~1000]
//	C	think[0~300]	think[300~600]		eat[600~900]		sleep[600~900]		think[900~1000]

//eat_time * 3 - (eat_time + sleep_time)

	while (is_termination(p))
	{
		pthread_mutex_unlock(p->m_t_flag_adr);
		if (!is_fork_available(p)) //&& !is_my_turn(p->manager_adr, p->idx))
		{ //홀수 일 때, 짝수 - 홀수 - 마지막 순으로 갈거다.
			*p->fork_stat[0] = 0;
			*p->fork_stat[1] = 0;
			pthread_mutex_unlock(p->m_fork_stat);

			if (!is_termination(p))
			{
				pthread_mutex_lock(p->m_fork_stat);
				*p->fork_stat[0] = 1;
				*p->fork_stat[1] = 1;
				pthread_mutex_unlock(p->m_fork_stat);
				return (0);
			}
			pthread_mutex_lock(p->m_time_adr);
			gettimeofday(time, NULL);
			temp = time->tv_sec * 1000 + time->tv_usec / 1000 - p->time_init_val;
			pthread_mutex_unlock(p->m_time_adr);
			pthread_mutex_lock(p->m_fork_slot[0]);
			printf("%llu %i has taken a fork.\n", temp, p->idx);
			pthread_mutex_lock(p->m_fork_slot[1]);
			printf("%llu %i has taken a fork.\n", temp, p->idx);
			//	printf("%i returning..\n", p->idx);
			pthread_mutex_unlock(p->m_t_flag_adr);

			if (grab_eat_sleep(p, time))
				break ;
		}
		else
			pthread_mutex_unlock(p->m_fork_stat);
	}
//	printf("%i returning..\n", p->idx);
	return (0);
}
