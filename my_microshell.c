#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

size_t	ft_strlen(char *str)
{
	int		i;

	i = 0;
	while (str[i])
		++i;
	return (i);
}

void	ft_error(char *err, char *str)
{
	write (1, err, ft_strlen(err) + 1);
	write (1, str, ft_strlen(str) + 1);
	write (1, "\n", 1);
}

int		find_pipe(char **argv)
{
	int		count;
	int		i;
	
	count = 0;
	i = -1;
	while (argv[++i])
		if (argv[i][0] == '|')
			++count;
	return (count);
}

void	launch_cd(int dot_start, char **argv)
{
	int		i;

	i = dot_start;
	while (argv[i] && argv[i][0] != ';')
		++i;
	if (i - dot_start == 0 || i - dot_start != 1)
		write(1, "error: cd: bad arguments\n", 26);
	if (chdir(argv[dot_start]) == -1)
		ft_error("error: cd: cannot change directory to ", argv[dot_start]);
}

void	ft_cd(int dot_start, char **argv, char **env, int ispipe)
{
	int		i;
	int		pid;
	int		status;

	i = dot_start;
	// printf("%s\n", argv[i]);
	if (argv[i][0] == 'c' && argv[i][1] == 'd')
		launch_cd(dot_start + 1, argv);
	else
	{
		if (ispipe)
		{
			if (execve(argv[i], argv + i, env) == -1)
				ft_error("error: cannot execute ", argv[i]);
		}
		else if ((pid = fork()) == 0)
		{
			if (execve(argv[i], argv + i, env) == -1)
				ft_error("error: cannot execute ", argv[i]);
		}
		else
			waitpid(pid, &status, 0);
	}
}

void	init_p(int **pipes, int i, int count_pipe)
{
	if (i < count_pipe)
	{
		pipes[i] = (int*)malloc(sizeof(int) * 3);
		pipe(pipes[i]);
	}
}

void	edit_fd(int **pipes, int i, int count_pipe)
{
	if (i > 0)
	{
		close(pipes[i - 1][1]);
		dup2(pipes[i - 1][0], 0);
	}
	if (i != count_pipe - 1)
	{
		close(pipes[i][0]);
		dup2(pipes[i][1], 1);
	}
}

void	close_fd(int **pipes, int i)
{
	if (i > 0)
	{
		close(pipes[i - 1][0]);
		close(pipes[i - 1][1]);
	}
}

void	free_waitpid(pid_t *pid, int count_pipe, int **pipes)
{
	int		i;
	int		status;

	i = -1;
	while (++i <= count_pipe)
	{
		waitpid(pid[i], &status, 0);
		if (i + 1 != count_pipe)
			free(pipes[i]);
	}
	free(pid);
	free(pipes);
}

int		search_pipe(char **argv, int dot_start)
{
	while (argv[dot_start])
	{
		if (argv[dot_start][0] == '|')
			return (dot_start);
		++dot_start;
	}
	return (dot_start);
}

void	start_pipe(int dot_start, char **argv, char **env, int count_pipe)
{
	pid_t	*pid;
	int	**pipes;
	int		i;
	int		pipe_null;

	pid = (pid_t*)malloc(sizeof(pid_t) * (count_pipe + 1));
	pipes = (int**)malloc(sizeof(int*) * (count_pipe + 1));
	pipes[count_pipe] = 0;
	i = -1;
	while (++i < count_pipe)
	{
		// printf("argv - %s\n", argv[dot_start]);
		init_p(pipes, i, count_pipe);
		pipe_null = search_pipe(argv, dot_start);
		if ((pid[i] = fork()) == 0)
		{
			edit_fd(pipes, i, count_pipe);
			// printf(">> %s\n", argv[pipe_null]);
			argv[pipe_null] = 0;
			ft_cd(dot_start, argv, env, 1);
			return ;
		}
		// if (i + 1 != count_pipe)
			dot_start = pipe_null + 1;
		close_fd(pipes, i);
	}
	free_waitpid(pid, count_pipe, pipes);
}

int     main(int argc, char **argv, char **env)
{
	int     i;
	int		dot_start;
	int     count_pipe;

	i = -1;
	dot_start = 1;
	count_pipe = find_pipe(argv);
	// printf(">>>>>argc - %d\n\n", argc);
	while (++i < argc)
	{
		// printf("argc - %d\n", argc);
		// printf("<>%d<>%s<>\n", i, argv[i]);
		if (!argv[i + 1] || (!strcmp(argv[i + 1], ";")))
		{
			argv[i + 1] = NULL;
			if (i + 1 > dot_start)
			{
				// printf(">>>\n");
				if (!count_pipe)
					ft_cd(dot_start, argv, env, 0);
				else
					start_pipe(dot_start, argv, env, count_pipe + 1);
			}
			dot_start = i + 2;	
		}
	}
	// printf("---");
	return (0);
}