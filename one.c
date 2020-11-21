#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int     ft_strlen(char *str)
{
	int     i;

	i = 0;
	while (str[i])
		++i;
	return (i);
}

void	error_fatal(int fd)
{
	write(fd, "error: fatal\n", 14);
	exit(1);
}

void	ft_error(char *err, char *str, int fd)
{
	write(fd, err, ft_strlen(err));
	write(fd, str, ft_strlen(str));
	write(fd, "\n", 1);
}

int		find_pipe(char **argv)
{
	int		i;
	int		count;

	i = -1;
	count = 0;
	while (argv[++i] && strcmp(argv[i], ";"))
	{
		if (argv[i][0] == '|')
			++count;
	}
	return (count);
}

void	launch_cd(char **argv, int dot_start, int fd)
{
	int		i;

	i = dot_start;
	while (argv[i] && argv[i][0] != ';')
		++i;
	if (i - dot_start != 1)
		write(2, "error: cd: bad arguments\n", 26);
	else if ((chdir(argv[dot_start])) != 0)
		ft_error("error: cd: cannot change directory to ", argv[dot_start], fd);
}

void	ft_cd(int dot_start, char **argv, char **env, int ispipe, int fd)
{
	int		i;
	int		pid;
	int		status;

	i = dot_start;
	if (!strcmp(argv[i], "cd"))
		launch_cd(argv, dot_start + 1, fd);
	else
	{
		if (ispipe)
		{
			if (execve(argv[i], argv + i, env) != 0)
				ft_error("error: cannot execute ", argv[i], fd);
		}
		else if ((pid = fork()) == 0)
		{
			if (execve(argv[i], argv + i, env) != 0)
				ft_error("error: cannot execute ", argv[i], fd);
		}
		else
			waitpid(pid, &status, 0);		
	}
}

void	init_p(int **pipes, int i, int count_pipe, int fd)
{
	if (i + 1 < count_pipe)
	{
		pipes[i] = (int*)malloc(sizeof(int) * 3);
		if (pipe(pipes[i]) < 0)
			error_fatal(fd);
	}
}

int		search_pipe(char **argv, int dot_start)
{
	while (argv[dot_start])
	{
		if (!strcmp(argv[dot_start], "|"))
			return (dot_start);
		++dot_start;
	}
	return (dot_start);
}

void	edit_fd(int **pipes, int i, int count_pipe, int fd)
{
	if (i > 0)
	{
		if (dup2(pipes[i - 1][0], 0) < 0)
			error_fatal(fd);
		close(pipes[i - 1][1]);
	}
	if (i + 1 != count_pipe)
	{
		if (dup2(pipes[i][1], 1) < 0)
			error_fatal(fd);
		close(pipes[i][0]);
	}
}

void	close_fd(int **pipes, int i)
{
	if (i > 0)
	{
		close(pipes[i - 1][1]);
		close(pipes[i - 1][0]);
	}
}

void	waitpid_free(int **pipes, pid_t *pid, int count_pipe)
{
	int		status;
	int		i;

	i = -1;
	while (++i <= count_pipe)
	{
		waitpid(pid[i], &status, 0);
		if (i + 1 != count_pipe)
			free(pipes[i]);
	}
	free(pipes);
	free(pid);
}

void	start_pipe(int dot_start, char **argv, char **env, int count_pipe, int fd)
{
	int		**pipes;
	pid_t	*pid;
	int		i;
	int		pipe_null;

	pid = (pid_t*)malloc(sizeof(pid_t) * (count_pipe + 1));
	pipes = (int**)malloc(sizeof(int*) * (count_pipe + 1));
	pipes[count_pipe] = 0;
	i = -1;
	while (++i < count_pipe)
	{
		init_p(pipes, i, count_pipe, fd);
		pipe_null = search_pipe(argv, dot_start);
		if ((pid[i] = fork()) < 0)
			error_fatal(fd);
		else if (pid[i] == 0)
		{
			edit_fd(pipes, i, count_pipe, fd);
			argv[pipe_null] = NULL;
			ft_cd(dot_start, argv, env, 1, fd);
			exit(0);
		}
		else if (i + 1 != count_pipe)
			dot_start = pipe_null + 1;
		close_fd(pipes, i);
	}
	waitpid_free(pipes, pid, count_pipe);
}

int		main(int argc, char **argv, char **env)
{
	int		fd;
	int		dot_start;
	int		i;
	int		count_pipe;

	fd = dup(2);
	i = -1;
	dot_start = 1;
	while(++i < argc)
	{
		if (!argv[i + 1] || !strcmp(argv[i + 1], ";"))
		{
			argv[i + 1] = NULL;
			count_pipe = find_pipe(argv + dot_start);
			if (i + 1 > dot_start)
			{
				if (!count_pipe)
					ft_cd(dot_start, argv, env, 0, fd);
				else
					start_pipe(dot_start, argv, env, count_pipe + 1, fd);
			}
			dot_start = i + 2;
		}
	}
}