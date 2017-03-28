#include <stlink/tools/flash.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bool starts_with(const char * str, const char * prefix) {
    size_t n = strlen(prefix);
    if(strlen(str) < n) return false;

    return (0 == strncmp(str, prefix, n));
}

int flash_get_opts(struct flash_opts* o, int ac, char** av)
{
    bool serial_specified = false;

    // defaults

    memset(o, 0, sizeof(*o));
    o->log_level = STND_LOG_LEVEL;

    // options

    while(ac >= 1) {
        if (strcmp(av[0], "--version") == 0) {
            printf("v%s\n", STLINK_VERSION);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(av[0], "--debug") == 0) {
            o->log_level = DEBUG_LOG_LEVEL;
        }
        else if (strcmp(av[0], "--reset") == 0) {
            o->reset = 1;
        }
        else if (strcmp(av[0], "--serial") == 0 || starts_with(av[0], "--serial=")) {
            const char * serial;
            if(strcmp(av[0], "--serial") == 0) {
                ac--;
                av++;
                if (ac < 1) return -1;
                serial = av[0];
            }
            else {
                serial = av[0] + strlen("--serial=");
            }

            /** @todo This is not really portable, as strlen really returns size_t we need to obey and not cast it to a signed type. */
            int j = (int)strlen(serial);
            int length = j / 2;  //the length of the destination-array
            if(j % 2 != 0) return -1;
 
            for(size_t k = 0; j >= 0 && k < sizeof(o->serial); ++k, j -= 2) {
                char buffer[3] = {0};
                memcpy(buffer, serial + j, 2);
                o->serial[length - k] = (uint8_t)strtol(buffer, NULL, 16);
            }

            serial_specified = true;
        }
        else if (strcmp(av[0], "--format") == 0 || starts_with(av[0], "--format=")) {
            const char * format;
            if(strcmp(av[0], "--format") == 0) {
                ac--;
                av++;
                if (ac < 1) return -1;
                format = av[0];
            }
            else {
                format = av[0] + strlen("--format=");
            }

            if (strcmp(format, "binary") == 0)
                o->format = FLASH_FORMAT_BINARY;
            else if (strcmp(format, "ihex") == 0)
                o->format = FLASH_FORMAT_IHEX;
            else
                return -1;
        }
	else if ( starts_with(av[0], "--flash=") ) {
		const char *arg = av[0] + strlen("--flash=");
		char *ep = 0;

		o->flash_size = (uint32_t)strtoul(arg,&ep,0);
		while ( *ep ) {
			switch ( *ep++ ) {
			case 0:
				break;
			case 'k':
			case 'K':
				o->flash_size *= 1024u;
				break;
			case 'm':
			case 'M':
				o->flash_size *= 1024u * 1024u;
				break;
			default:
				fprintf(stderr,"Invalid --flash=%s\n",arg);
				return -1;
			}
		}
	}
	else {
            break;  // non-option found
        }

        ac--;
        av++;
    }

    // command and (optional) device name

    while(ac >= 1) { // looks like for stlinkv1 the device name and command may be swaped - check both positions in all cases
        if (strcmp(av[0], "erase") == 0) {
            if (o->cmd != FLASH_CMD_NONE) return -1;
            o->cmd = FLASH_CMD_ERASE;
        }
        else if (strcmp(av[0], "read") == 0) {
            if (o->cmd != FLASH_CMD_NONE) return -1;
            o->cmd = FLASH_CMD_READ;
        }
        else if (strcmp(av[0], "write") == 0) {
            if (o->cmd != FLASH_CMD_NONE) return -1;
            o->cmd = FLASH_CMD_WRITE;
        }
        else if (strcmp(av[0], "reset") == 0) {
            if (o->cmd != FLASH_CMD_NONE) return -1;
            o->cmd = CMD_RESET;
        }
        else if(starts_with(av[0], "/dev/")) {
            if (o->devname) return -1;
            o->devname = av[0];
        }
        else {
            break;
        }

        ac--;
        av++;
    }

    char * tail;
    switch(o->cmd) {
        case FLASH_CMD_NONE:     // no command found
            return -1;

        case FLASH_CMD_ERASE:    // no more arguments expected
            if(ac != 0) return -1;
            break;

        case FLASH_CMD_READ:     // expect filename, addr and size
            if (ac != 3) return -1;

            o->filename = av[0];
            o->addr = (uint32_t) strtoul(av[1], &tail, 16);
            if(tail[0] != '\0') return -1;

            o->size = strtoul(av[2], &tail, 16);
            if(tail[0] != '\0') return -1;

            break;

        case FLASH_CMD_WRITE:
            if(o->format == FLASH_FORMAT_BINARY) {    // expect filename and addr
                if (ac != 2) return -1;

                o->filename = av[0];
                o->addr = (uint32_t) strtoul(av[1], &tail, 16);
                if(tail[0] != '\0') return -1;
            }
            else if(o->format == FLASH_FORMAT_IHEX) { // expect filename
                if (ac != 1) return -1;

                o->filename = av[0];
            }
            else {
                return -1;
            }
            break;

       default: break ;
    }

    // some constistence checks
    
    if(serial_specified && o->devname != NULL) return -1; // serial not supported for v1

    return 0;
}

