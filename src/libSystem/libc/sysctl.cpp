#include "config.h"
#include "sysctl.h"
#include "log.h"
#include "trace.h"
#include "darwin_errno_codes.h"
#include <errno.h>
#include <unistd.h>
#include <ctime>
#include <sys/sysinfo.h>
#include <sys/time.h>

#define CTL_KERN 1
#define CTL_HW 6

#define HW_NCPU 3
#define HW_PHYSMEM 5
#define HW_USERMEM 6
#define HW_MEMSIZE 24
#define HW_AVAILCPU 25

int __darwin_sysctlbyname(const char* name, void* oldp, size_t* oldlenp, void* newp, size_t newlen)
{
	TRACE5(name, oldp, oldlenp, newp, newlen);

	uint64_t val;

	if (newp)
	{
		LOG << "sysctl with newp isn't supported yet.\n";
		errno = DARWIN_EINVAL;
		return -1;
	}

	/*
	if (*oldlenp != 4 && *oldlenp != 8)
	{
		LOG << "sysctl(HW) with oldlenp=" << *oldlenp << " isn't supported yet.\n";
		errno = DARWIN_EINVAL;
		return -1;
	}
	*/

	if (!strcmp(name, "hw.physicalcpu") || !strcmp(name, "hw.logicalcpu") || !strcmp(name, "hw.ncpu"))
		val = ::sysconf(_SC_NPROCESSORS_ONLN);
	else if (!strcmp(name, "hw.physicalcpu_max") || !strcmp(name, "hw.logicalcpu_max"))
		val = ::sysconf(_SC_NPROCESSORS_CONF);
	else if (!strcmp(name, "kern.boottime"))
	{
		struct timeval tv;
		struct sysinfo si;
		
		if (*oldlenp < sizeof(struct timeval))
		{
			errno = DARWIN_ENOMEM;
			return -1;
		}
		
		sysinfo(&si);
		gettimeofday(&tv, nullptr);
		tv.tv_sec -= si.uptime;
		
		*oldlenp = sizeof(struct timeval);
		memcpy(oldp, &tv, sizeof(tv));
		return 0;
	}
	else
	{
		errno = DARWIN_EINVAL;
		return -1;
	}

	if (oldp)
	{
		if (*oldlenp == 4)
			*reinterpret_cast<uint32_t*>(oldp) = val;
		else if (*oldlenp == 8)
			*reinterpret_cast<uint64_t*>(oldp) = val;
	}
	else
		*oldlenp = sizeof(long);
	return 0;	
}

int __darwin_sysctl(int* name, unsigned int namelen,
                    void* oldp, size_t* oldlenp,
                    void* newp, size_t newlen)
{
	TRACE6(name, namelen, oldp, oldlenp, newp, newlen);
	
	LOG << "sysctl: namelen=" << namelen;
	for (int i = 0; i < namelen; i++)
		LOG << " name[" << i << "]=" << name[i];
	LOG << " newp=" << newp << std::endl;

	if (newp)
	{
		LOG << "sysctl with newp isn't supported yet.\n";
		errno = DARWIN_EINVAL;
		return -1;
	}

	if (namelen != 2)
	{
		LOG << "sysctl with namelen=" << namelen << " isn't supported yet.\n";
		errno = DARWIN_EINVAL;
		return -1;
	}

	switch (name[0])
	{
	case CTL_HW:
	{
		if (*oldlenp != 4 && *oldlenp != 8)
		{
			LOG << "sysctl(HW) with oldlenp=" << *oldlenp << " isn't supported yet.\n";
			errno = DARWIN_EINVAL;
			return -1;
		}

		uint64_t val = 0;
		switch (name[1])
		{
		case HW_NCPU:
			val = ::sysconf(_SC_NPROCESSORS_CONF);
			break;
		case HW_AVAILCPU:
			val = ::sysconf(_SC_NPROCESSORS_ONLN);
			break;
		case HW_PHYSMEM:
		case HW_USERMEM:
		case HW_MEMSIZE:
		{
			long pages = ::sysconf(_SC_PHYS_PAGES);
			long page_size = ::sysconf(_SC_PAGE_SIZE);
			val = pages * page_size;
			break;
		}

		default:
			LOG << "sysctl(HW) with name[1]=" << name[1] << " isn't supported yet.\n";
        
			errno = DARWIN_EINVAL;
			return -1;
		}

		if (oldp)
		{
			if (*oldlenp == 4)
				*reinterpret_cast<uint32_t*>(oldp) = val;
			else if (*oldlenp == 8)
				*reinterpret_cast<uint64_t*>(oldp) = val;
		}
		else
			*oldlenp = sizeof(long);
		return 0;
    }

	case CTL_KERN:
	{
		switch (name[1])
		{
			case KERN_OSRELEASE:
				
				// TODO: use real uname?
				if (oldp)
					strcpy((char*)oldp, "10.7.0");
				*oldlenp = 7;
				break;
				
			case KERN_OSVERSION:
				if (oldp)
					strcpy((char*)oldp, "10J869");
				*oldlenp = 7;
				break;
				
			case KERN_BOOTTIME:
			{
				struct timeval tv;
				struct sysinfo si;
				
				if (*oldlenp < sizeof(struct timeval))
				{
					errno = DARWIN_ENOMEM;
					return -1;
				}
				
				sysinfo(&si);
				gettimeofday(&tv, nullptr);
				tv.tv_sec -= si.uptime;
				
				*oldlenp = sizeof(struct timeval);
				memcpy(oldp, &tv, sizeof(tv));
				return 0;
			}

			default:
				LOG << "sysctl(KERN) with name[1]=" << name[1] << " isn't supported yet.\n";
				errno = DARWIN_EINVAL;
				return -1;
		}
		return 0;
	}

	default:
		LOG << "sysctl with name[0]=" << name[0] << " isn't supported yet.\n";
		errno = DARWIN_EINVAL;
		return -1;
	}
}
