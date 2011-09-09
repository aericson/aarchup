/* 
   Copyright 2010 Rorschach <r0rschach@lavabit.com>,
   2011 Andrew Kravchuk <awkravchuk@gmail.com>,
   2011 aericson <de.ericson@gmail.com>
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. 
   */

#include <libnotify/notify.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <stdbool.h>

#define AUR_HEADER "AUR updates:\n"
#define STREQ !strcmp
#define VERSION_NUMBER "1.6.6"


/* Parse cower -u output. 
 * Format: ":: package_name version -> new_version" */
void parse(char *line){
    int i, j;
    int llen = strlen(line);
    /* Find first space */
    for(i=0;!isspace(line[i]) && i < llen; ++i);
    /* Copy package_name */
    for(j=0, ++i;!isspace(line[i]) && i < llen;++j, ++i)
        line[j] = line[i];
    i = llen - 1;
    line[j++] = ' ';
    /* Find first char of new_version */
    while(line[i]!=' ')
        --i;
    for(i++;line[i] != '\0'; ++i)
        line[j++] = line[i];
    line[j] = '\0';
}

/*
 * Splits string in spaces.
 * Does not treat NULL or empty string
 */
char ** split(char *c, int *size){
    char **ls = (char **) malloc(sizeof(char*));
    int i, j=0, allocd=1;
    *size = 0;
    
    for(i=0;i<strlen(c)+1;++i){
        if(isspace(c[i])|| c[i] == '\0'){
            if((*size) >= allocd){
                ls = (char **)realloc(ls, sizeof(char*)*allocd*2);
                allocd *= 2;
            }       
            if(i!=j){
                ls[(*size)] = (char *)malloc(i-j+1);
                strncpy(ls[(*size)], c+j, i-j);
                ls[(*size)][i-j] = '\0'; 
                (*size)++;
                j=i;
            }       
            j++;
        }       
    }       
    return ls;
}

/*
 * Check if package in list
 */
int inlist(char **list, char *elem, int size){
    int i;
    for(i=0;i<size;++i){
        if(STREQ(list[i],elem)){
            return TRUE;
        }       
    }       
    return FALSE;
}

/*
 * Check if s1 starts with s2
 */
int startswith(char *s1, char *s2){
    int llen = strlen(s2);
    int i;
    for(i=0;i<llen;++i)
        if(s1[i]!=s2[i])
            return FALSE;
    return TRUE;
}

/*
 * Parse p_file and get the value of IgnorePkg
 */
char* parse_conf(FILE *p_file){
    int i;
    char buf[BUFSIZ];
    while(fgets(buf, BUFSIZ, p_file)){
        if(startswith(buf, "IgnorePkg")){
            char *c = buf;
            while(*c != '=')
                c++;
            c++;
            char *d = (char*)malloc(strlen(buf));
            for(i=0;c[i]!='\n'&&c[i]!='#';++i)
                d[i] = c[i];
            d[i] = '\0';
            return d;
        }
    }
    return NULL;
}

char **get_ignore_pkgs(int *ign_pkg_size, int debug){
    FILE *pacman;
    pacman = fopen("/etc/pacman.conf", "rt");
    if(!pacman){
        printf("DEBUG(error): Impossible to open /etc/pacman.conf\n");
        return NULL;
    }
    char *value = parse_conf(pacman);
    fclose(pacman);
    char **IgnorePkg = NULL;
    if(value){
        IgnorePkg = split(value, ign_pkg_size);
        free(value);
    }
    return IgnorePkg;    
}

void free_mat(char ***mat, int size){
    int i;
    for(i=0;i<size;++i)
        free((*mat)[i]);
    free(*mat);
    *mat = NULL;
}

void read_update_pipe(FILE *pac_out, int *update_count, int max_number_out, int debug,
                      char **IgnorePkg, int ign_pkg_size, char **output_string,
                      int ignore_pkg_flag, bool *got_updates, int aur){
    char line[BUFSIZ];
    int llen;
    int first = TRUE;
    int ignore_index = aur ? 1 : 0;
    while(fgets(line,BUFSIZ,pac_out)){
        /* We leave the loop if we have more updates than we want to show in the notification. */
        if (*update_count >= max_number_out)
        {
            if(debug)
                printf("DEBUG(info): Maximum number of updates to list reached, stopping\n");
            break;
        }
        if(ignore_pkg_flag && IgnorePkg){
            int spl_size;
            char **splitted;
            splitted = split(line, &spl_size);
            if(ignore_index < ign_pkg_size && inlist(IgnorePkg, splitted[ignore_index], ign_pkg_size)){
                if(debug)
                    printf("DEBUG(info): Ignoring package %s.\n", splitted[ignore_index]);
                free_mat(&splitted, spl_size);
                continue;
            }
            free_mat(&splitted, spl_size);
        }
        if(aur && first){
            llen = strlen(AUR_HEADER);
            *output_string = (char *)realloc(*output_string,strlen(*output_string)+1+llen);
            strncat(*output_string, AUR_HEADER, llen);
            first = FALSE;
        }
        (*update_count)++;
        if(aur)
            parse(line);
        else{
            if(strlen(line)+2 <= BUFSIZ){
                line[strlen(line)-1] = '\0';
                strcat(line, "+\n");
            }
        }
        if(debug){
            if(!aur)
                printf("DEBUG(info): Found update %s", line);
            else
                printf("DEBUG(info): Found update in aur %s", line);
            if(line[strlen(line)-1] != '\n')
                printf("\n");
        }
        /* If we are in this loop, we got updates waiting. */
        *got_updates = TRUE;
        /* We get the length of the current line. */
        llen = strlen(line);
        /* We allocate that much more memory+2 bytes for the "- "+1 byte as delimiter. */
        *output_string = (char *)realloc(*output_string,strlen(*output_string)+1+llen+2);
        /* We add the line to the output string. */
        strncat(*output_string,"- ",2);
        strncat(*output_string,line,llen);
    }
}

/* Prints the help. */
int print_help(char *name)
{
    printf("Usage: %s [options]\n\n",name);
    printf("Options:\n");
    printf("          --command|-c [value]        Set the command which gives out the list of updates.\n");
    printf("                                      The default is /usr/bin/pacman -Qu\n");
    printf("          --icon|-p [value]           Shows the icon, whose path has been given as value, in the notification.\n");
    printf("                                      By default no icon is shown.\n");
    printf("          --maxentries|-m [value]     Set the maximum number of packages which shall be displayed in the notification.\n");
    printf("                                      The default value is 30.\n");
    printf("          --timeout|-t [value]        Set the timeout after which the notification disappers in seconds.\n");
    printf("                                      The default value is 3600 seconds, which is 1 hour.\n");
    printf("          --uid|-i [value]            Set the uid of the process.\n");
    printf("                                      The default is to keep the uid of the user who started aarchup.\n");
    printf("                                      !!You should change this if root is executing aarchup!!\n");
    printf("          --urgency|-u [value]        Set the libnotify urgency-level. Possible values are: low, normal and critical.\n");
    printf("                                      The default value is normal. With changing this value you can change the color of the notification.\n");
    printf("          --loop-time|-l [value]      Using this the program will loop indefinitely. Just use this if you won't use cron.\n");
    printf("                                      The value should be the number of minutes from each update check, if none is informed 60 will be used.\n");
    printf("                                      For more information on it check man.\n");
    printf("          --help                      Prints this help.\n");
    printf("          --version                   Shows the version.\n");
    printf("          --aur                       Check aur for new packages too. Will need cower installed.\n");
    printf("          --debug|-d                  Print debug info.\n");
    printf("          --ftimeout|-f [value]       Program will manually enforce timeout for closing notification.\n");
    printf("                                      Do NOT use with --timeout, if --timeout works or without --loop-time [value].\n");
    printf("                                      The value for this option should be in minutes.\n");
    printf("          --ignore-disconnect         If this flag is set aarchup will notify you about new updates even if pacman -Sy failed.\n");
    printf("                                      no point in using this without --loop-time.\n");
    printf("          --pkg-no-ignore             If this flag is set will not use the IgnorePkg variable from pacman.conf. Without the flag will ignore those\n");
    printf("                                      packages\n");
    printf("\nMore informations can be found in the manpage.\n");
    exit(0);
}

/* Prints the version. */
int print_version()
{
    printf("aarchup %s\n",VERSION_NUMBER);
    printf("Copyright 2011 aericson <de.ericson@gmail.com>\n");
    printf("Copyright 2010 Rorschach <r0rschach@lavabit.com>,\n2011 Andrew Kravchuk <awkravchuk@gmail.com>\n");
    printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
    printf("This is free software: you are free to change and redistribute it.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}


int main(int argc, char **argv)
{
    NotifyUrgency urgency;


    /* Default timeout-value is: 60 min (int timeout = 3600*1000;)
       After this time the desktop notification disappears */
    int timeout = 3600*1000;
    /* Restricts the number of packages which should be included in the desktop notification.*/
    int max_number_out = 30;
    int loop_time = 3600;
    int manual_timeout = 0;
    static int help_flag = 0;
    static int version_flag = 0;
    static int aur = 0;
    static int ignore_disc_flag = 0;
    static int ignore_pkg_flag = 1;
    bool will_loop = FALSE, debug = FALSE, argv_dealloc = FALSE;
    /* Sets the urgency-level to normal. */
    urgency = NOTIFY_URGENCY_NORMAL;
    /* The default command to get a list of packages to update. */
    char *command = "/usr/bin/pacman -Qu";
    /* The default icon to show: none */
    gchar *icon = NULL;
    
    /* Try to read configuration file. */
    GKeyFile *p_key_file;
    GError *error = NULL;
    p_key_file = g_key_file_new();
    if(!g_key_file_load_from_file(
                p_key_file, "/etc/aarchup.conf",
                G_KEY_FILE_NONE,
                &error)){
        if(error->code != G_FILE_ERROR_NOENT){
            g_warning("Impossible to load config file: %s\n", error->message);
            g_error_free(error);
            g_key_file_free(p_key_file);
            exit(EXIT_FAILURE);
        }
    }
    /* If config file load the args to argv */
    if(!error){
        gchar **k;
        gsize c;
        gchar *value;
        /* check if --version or --help */
        if(argc > 1){
            if(strcmp(argv[1], "--version") == 0)
                print_version();
            if(strcmp(argv[1], "--help") == 0)
                print_help(argv[0]);
        }
        /* Now we get every key from aarchup.conf */
        k = g_key_file_get_keys(p_key_file, "main", &c, NULL);
        int i = 0;
        /* Will simulate argv each argument should have at most 2 spot(key/value)
         * need also to allocate 1 spot for argv[0] */
        char **argvn = (char **)malloc(sizeof(char *)*(c*2+1));
        argv_dealloc = TRUE;
        argvn[0] = malloc(strlen(argv[0])+1);
        strcpy(argvn[0], argv[0]);
        argc = 1;
        argv = argvn;
        for(;k[i];++i){
            /* size should be strlen + 2("--") + 1('\0')*/
            argv[argc] = malloc(strlen(k[i]) + 3);
            sprintf(argv[argc], "--%s", k[i]);
            argc++;
            /* get value for the argument k[i] */
            value = g_key_file_get_string(p_key_file, "main", k[i], NULL);
            if(value && strlen(value)> 0){
                /* Strip begging and ending " if starts with it */
                if(value[0]=='"'){
                    int j;
                    for(j=0;j<strlen(value)-1;++j)
                        value[j] = value[j+1];
                    value[strlen(value)-2] = '\0';                   
                }
                argv[argc] = malloc(strlen(value) + 1);
                strcpy(argv[argc],value);
                argc++;
                g_free(value);
            }
        }
        g_strfreev(k);
    } else {
        g_error_free(error);
    }
    g_key_file_free(p_key_file);
    error = NULL;

    /* We parse the commandline options. */
    int c;
    while(1){
        /* Long opts */
        static struct option long_options[] = {
            {"command", required_argument, 0, 'c'},
            {"icon", required_argument, 0, 'p'},
            {"maxentries", required_argument, 0, 'm'},
            {"timeout", required_argument, 0, 't'},
            {"uid", required_argument, 0, 'i'},
            {"urgency", required_argument, 0, 'u'},
            {"loop-time", required_argument, 0, 'l'},
            {"help", no_argument, &help_flag, 1},
            {"version", no_argument, &version_flag, 1},
            {"aur", no_argument, &aur, 1},
            {"ftimeout", required_argument, 0, 'f'},
            {"debug", no_argument, 0, 'd'},
            {"ignore-disconnect", no_argument, &ignore_disc_flag, 1},
            {"pkg-no-ignore", no_argument, &ignore_pkg_flag, 0},
            {0, 0, 0, 0},
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "c:p:m:t:i:u:l:df:", long_options,
                &option_index);
        if(c == -1)
            break;
        /* Short opts */
        switch(c)
        {
            case 0:
                if(long_options[option_index].flag)
                    break;
                if(debug){
                    printf("DEBUG(error): erroropt: option %s", long_options[option_index].name);
                    if(optarg)
                        printf(" with args %s", optarg);
                    printf("\n");
                }
                break;
            case 'c':
                command = optarg;
                if(debug)
                    printf("DEBUG(info): command set: %s\n", command);
                break;
            case 'p':
                icon = optarg;
                if(debug)
                    printf("DEBUG(info): icon set: %s\n", icon);
                break;
            case 'm':
                if(!isdigit(optarg[0])){
                    printf("--maxentries argument needs to be a number\n");
                    exit(1);
                }
                max_number_out = atoi(optarg);
                if(debug)
                    printf("DEBUG(info): max_number set: %i\n", max_number_out);
                break;
            case 't':
                if(!isdigit(optarg[0])){
                    printf("--timeout argument needs to be a number\n");
                    exit(1);
                }
                timeout = atoi(optarg)*1000;
                if(debug)
                    printf("DEBUG(info): timeout set: %i\n", timeout/1000);
                break;
            case 'i':
                if(!isdigit(optarg[0])){
                    printf("--uid argument needs to be a number\n");
                    exit(1);
                }
                if ( setuid(atoi(optarg)) != 0 )
                {
                    printf("Couldn't change to the given uid!\n");
                    exit(1);
                }
                if(debug)
                    printf("DEBUG(info): uid setted.\n");
                break;
            case 'u':
                if ( strcmp(optarg, "low") == 0 )
                {
                    urgency=NOTIFY_URGENCY_LOW;
                }
                else if ( strcmp(optarg, "normal") == 0 )
                {
                    urgency=NOTIFY_URGENCY_NORMAL;
                }        
                else if ( strcmp(optarg, "critical") == 0 )
                {
                    urgency=NOTIFY_URGENCY_CRITICAL;
                }
                else{
                    printf("--urgency has to be 'low', 'normal' or 'critical\n");
                    exit(1);
                }
                if(debug)
                    printf("DEBUG(info): urgency set: %i\n", urgency);
                break;
            case 'l':
                will_loop = TRUE;
                if(!isdigit(optarg[0])){
                    printf("--loop-time argument needs to be a number\n");
                    exit(1);
                }
                loop_time = atoi(optarg) * 60;
                if(debug)
                    printf("DEBUG(info): loop_time set: %i\n", loop_time/60);
                break;
            case 'd':
                debug = TRUE;
                if(debug)
                    printf("DEBUG(info): debug on\n");
                break;
            case 'f':
                if(!isdigit(optarg[0])){
                    printf("--ftimeout argument needs to be a number\n");
                    exit(1);
                }
                manual_timeout = atoi(optarg) * 60;
                if(!will_loop){
                    printf("--ftimeout can't be used without or before --loop-time\n");
                    exit(1);
                }
                if(manual_timeout > loop_time){
                    printf("Please set a value for --ftimeout that is higher than --loop-time\n");
                    exit(1);
                }
                if(debug)
                    printf("DEBUG(info): manual_timeout: %i\n", manual_timeout/60);
                break;
            case '?':
                print_help(argv[0]);
                break;
            default:
                exit(1);
        }        
    }

    if(debug && !argv_dealloc)
        printf("DEBUG(info): /etc/aarchup.conf was not found using args instead.\n");
    if(version_flag)
        print_version();
    if(help_flag)
        print_help(argv[0]);
    if(debug && aur)
        printf("DEBUG(info): aur is on\n");
    if(debug && ignore_disc_flag)
        printf("DEBUG(info): ignore-diconnect is on. Will ignore 'pacman -Sy' failure.\n");
    if(debug && !ignore_pkg_flag)
        printf("DEBUG(info): ignoring pacman.conf IgnorePkg variable.\n");

    /* Those are needed by libnotify. */
    char *name = "New Updates";
    char *category = "update";
    char *cower = "cower -u";
    NotifyNotification *my_notify = NULL;

    /* Those are needed for the output. */
    char *output_string; 
    bool got_updates;

    /* Those are needed for getting the list of updates. */
    FILE *pac_out;
    
    /* For debug */
    time_t now;

    /* For manual timeout */
    int offset = 0;
    
    /* For IgnorePkg */
    gchar **IgnorePkg = NULL;;
    int ign_pkg_size;

    do{
        /* If isn't set to loop than probably pacman's database was alreald synced  */
        if(will_loop){
            if(debug)
                printf("DEBUG(info): loop-time is on, running pacman -Sy\n");
            int success = system("sudo pacman -Sy &> /dev/null");
            if(success != 0){
                if(debug){
                    printf("DEBUG(error): failed to do pacman -Sy\n");
                    printf("DEBUG(error): it returned %i\n", success);
                }
                if(!ignore_disc_flag){
                    if(debug){
                        time(&now);
                        printf("DEBUG(info): Time now %s", ctime(&now));
                        printf("DEBUG(info): Next run will be in %i minutes\n", (loop_time-offset)/60);
                    }
                    fflush(stdout); // Make sure that all output was printed(needed if output is redirected to a log file)
                    sleep(loop_time-offset);
                    offset = 0;
                }
            }
        }
        /* We get stdout of pacman -Qu into the pac_out stream.
           Remember we can't use fseek(stream,0,SEEK_END) with 
           popen-streams, thus we are reading BUFSIZ sized lines
           and allocate dynamically more memory for our output. */  
        if(debug)
            printf("DEBUG(info): running command: %s\n", command);
        pac_out = popen(command,"r");
        got_updates = FALSE;
        output_string = malloc(24);
        sprintf(output_string,"There are updates for:\n");
        int i;
        i = 0;
        /* update check */
        if(ignore_pkg_flag)
            IgnorePkg = get_ignore_pkgs(&ign_pkg_size,debug);
        
        read_update_pipe(pac_out, &i, max_number_out, debug, IgnorePkg, ign_pkg_size, &output_string,
                        ignore_pkg_flag, &got_updates, FALSE);
        
        if(ignore_pkg_flag && IgnorePkg)
            free_mat(&IgnorePkg, ign_pkg_size);
        /* We close the popen stream if we don't need it anymore. */
        pclose(pac_out);
        /* aur check */
        if(aur && i < max_number_out){
            if(debug)
                printf("DEBUG(info): aur is on, checking for updates in aur\n");
            /* call cower */
            if(debug)
                printf("DEBUG(info): running command: %s\n", cower);
            pac_out = popen(cower, "r");
            if(ignore_pkg_flag)
                IgnorePkg = get_ignore_pkgs(&ign_pkg_size,debug);
            
            read_update_pipe(pac_out, &i, max_number_out, debug, IgnorePkg, ign_pkg_size,
                    &output_string, ignore_pkg_flag, &got_updates, TRUE);
            
            if(ignore_pkg_flag && IgnorePkg)
                free_mat(&IgnorePkg, ign_pkg_size);
            
            pclose(pac_out);
        }
        /* If we got updates we are showing them in a notification */
        if (got_updates == TRUE)
        {
            if(debug)
                printf("DEBUG(info): Got updates will show notification\n");
            /* Initiates the libnotify when needed. */	
            if(!notify_is_initted())
                notify_init(name);
            /* Removes the last newlinefeed of the output_string, if the last sign is a newlinefeed. */
            if ( output_string[strlen(output_string)-1] == '\n' )
            {
                output_string[strlen(output_string)-1] = '\0';
            }
            /* Constructs the notification or update.
             * If fails to update previous notificaion
             * will try to create a new one.
             * loop will only run twice at max; */
            bool persist = TRUE;
            bool success;
            /* Loop to try again if error on showing notification. */
            do{
                if(!my_notify){
                    my_notify = notify_notification_new("New updates for Arch Linux available!",output_string,icon);
                }
                else
                    notify_notification_update(my_notify, "New updates for Arch Linux available!",output_string,icon);
                /* Sets the timeout until the notification disappears. */
                notify_notification_set_timeout(my_notify,timeout);
                /* We set the category.*/
                notify_notification_set_category(my_notify,category);
                /* We set the urgency, which can be changed with a commandline option */
                notify_notification_set_urgency (my_notify,urgency);
                /* We finally show the notification, */	
                success = notify_notification_show(my_notify,&error);
                if(success && debug)
                    printf("DEBUG(info): Notification was shown with success.\n");
                else if(!success){
                    if(debug){
                        printf("DEBUG(error): Notification failed, reason:\n\t");
                        printf("[%i]%s\n", error->code, error->message);
                        g_error_free(error);
                        error = NULL;
                    }
                    if(persist){
                        printf("DEBUG(error): It could have been caused by an enviroment restart.\n");
                        printf("DEBUG(error): Trying to work around it by reinitin libnotify.\n");
                        my_notify = NULL;
                        notify_uninit();
                        notify_init(name);
                        persist = FALSE;
                    }
                }
            } while(!my_notify);
            if(error){
                g_error_free(error);
                error = NULL;
            }
            if(manual_timeout && success && will_loop){
                if(debug)
                    printf("DEBUG(info): Will close notification in %i minutes(this time will be reduced from the loop-time).\n", manual_timeout/60);
                fflush(stdout); // Make sure that all output was printed(needed if output is redirected to a log file)
                sleep(manual_timeout);
                offset = manual_timeout;
                bool success = notify_notification_close(my_notify, &error);
                if(debug){
                    if(success)
                        printf("DEBUG(info): Notification closed.\n");
                    else{
                        printf("DEBUG(error): Failed to close, reason:\n\t");
                        printf("[%i]%s\n", error->code, error->message);
                    }
                }
                if(error){
                    g_error_free(error);
                    error = NULL;
                }
            }
        } else {
            if(debug)
                printf("DEBUG(info): No updates found.\n");
            if(my_notify){
                if(debug)
                    printf("DEBUG(info): Previous notification found. Closing it in case it was still opened.\n");
                bool success = notify_notification_close(my_notify, &error);
                if(debug){
                    if(success)
                        printf("DEBUG(info): Notification closed.\n");
                    else{
                        printf("DEBUG(error): Failed to close, reason:\n\t");
                        printf("[%i]%s\n", error->code, error->message);
                    }
                }
                if(error){
                    g_error_free(error);
                    error = NULL;
                }
            }

        }
        /* Should be safe now */
        free(output_string);
        if(will_loop){
            if(debug){
                time(&now);
                printf("DEBUG(info): Time now %s", ctime(&now));
                printf("DEBUG(info): Next run will be in %i minutes\n", (loop_time-offset)/60);
            }
            fflush(stdout); // Make sure that all output was printed(needed if output is redirected to a log file)
            sleep(loop_time-offset);
            offset = 0;
        }
    }while(will_loop);
    if(argv_dealloc)
        free_mat(&argv, argc);
    /* and deinitialize the libnotify afterwards. */    
    notify_uninit();

    return 0;
}
