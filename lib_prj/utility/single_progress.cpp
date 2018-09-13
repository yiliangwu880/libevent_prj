#include "single_progress.hpp"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include "logFile.h" //目前调试用，移植删掉

file_lock::file_lock(const std::string& file_name)
:m_fd(-1)
{
	m_pid_file = "./" + file_name + ".lock";
}

file_lock::~file_lock()
{
	unlock();
}

//尝试给 文件锁拥有进程发信号
int file_lock::kill(int sig)
{
	int fd = open(m_pid_file.c_str(), O_TRUNC | O_RDWR);
	if (fd < 0)
	{
		LOG_ERROR("open fail fail, open fd = %d, %d\n", fd, sig);
		return -1;
	}

	flock flock_data;
	flock_data.l_type = F_WRLCK;
	flock_data.l_start = 0;
	flock_data.l_whence = SEEK_SET;
	flock_data.l_len = 0;
	//判断整个文件是否有写锁（F_WRLCK）
	if (fcntl(fd, F_GETLK, reinterpret_cast<long>(&flock_data)) == -1)
	{
		LOG_ERROR("fcntl error, fd = %d, %d\n", fd, sig);
		return -1;
	}

	if (flock_data.l_type == F_UNLCK)//case no lock
	{
		LOG_ERROR("file no lock");
		return 0;
	}
	//注意，这个过程不是原子操作，过程有可能锁状态变化~！

	//情况：有锁，把现有锁信息已经写到 flock_data
	// if (::kill(lock.l_pid, sig) == -1)
	// 这里暂时写死，Kill, unblockable (POSIX).
	if (::kill(flock_data.l_pid, sig) == -1)
	{
		LOG_ERROR("file_lock::kill ::kill(lock.l_pid, sig) == -1, pid = %d, sig = %d, errno = %s, %d", flock_data.l_pid, sig, strerror(errno), errno);
		return -1;
	}
	else
	{
		return 1;
	}
}

bool file_lock::lock()
{
	m_fd = open(m_pid_file.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
	if (m_fd < 0)
	{
		LOG_ERROR("base -> dispatch_events daemon failure! m_fd = %d,%s(%d)", m_fd, strerror(errno), errno);
		return false;
	}
	int lock_result = lockf(m_fd, F_TEST, 0);
	if (lock_result < 0)
	{
		// lockf F_TEST failure.
		LOG_ERROR("base -> dispatch_events daemon failure!F_TEST lock_result = %d", lock_result);
		return false;
	}
	lock_result = lockf(m_fd, F_LOCK, 0);
	if (lock_result < 0)
	{
		// lockf F_LOCK failure.
		LOG_ERROR("base -> dispatch_events daemon failure!F_LOCK lock_result = %d", lock_result);
		return false;
	}
	return true;
}

void file_lock::unlock()
{
	if (-1 == m_fd )
	{
		return;
	}
	lockf(m_fd, F_ULOCK, 0);
}

SingleProgress::SingleProgress()
:m_is_exit(false)
{

}


void SingleProgress::CheckSingle(const std::string &single_file_name)
{
	static file_lock m_file_lock(single_file_name);
	if (!m_file_lock.lock())
	{
		LOG_DEBUG("progress aleady run!");
		exit(1);
	}

	signal(SIGUSR1, SingleProgress::catch_signal);
}

void SingleProgress::StopSingle(const std::string &single_file_name)
{
	static file_lock m_file_lock(single_file_name);
	if (!m_file_lock.kill(SIGUSR1))
	{
		LOG_DEBUG("can't find runing progress to stop");
		return;
	}
}

void SingleProgress::catch_signal(int sig_type)
{
	SingleProgress::Instance().m_is_exit = true;
}
