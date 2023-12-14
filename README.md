# simple-logger
a simple async cpp linux logger that works with c and cpp.

## usage
you can add src to you SRCS and include to your include paths or use https://github.com/Itai-lupo/cpp-project-template
you should also include git@github.com:Itai-lupo/simple-error-check.git and https://github.com/Itai-lupo/simple-files in your project includes.

this our the defines that can be used to change the behaver:


- CLOCK_TO_USE
    the system clock to use default to CLOCK_REALTIME_COARSE

- FILE_ID 
    this should be a uniq int for each of you files default to -1

- PRINT_FUNCTION(logData) 
    this is the function that is used to print the logInfo_t struct default to writeLog(logData)


- LOG_MACRO(severity, msg, ...) 
    this is the base log macro all the logs use it, you probably don't want to change it unless you know exactly what you are doing

- LOG_<level>(msg, ...)
    for each of the log levels this is the macro you should call to log it, 
    you can change it if you want to use a diffrent logic for one of the log levels.
    for exmple exit on critical logs

- FORMAT_FUNC(logData, format, ...) 
    this is the macro the format the string you pass to the log and put it in the log data struct,
    default to ether fmt::format or snprintf format.
- USE_FMT_LOG
    define that to use fmt formatter will be define by default on cpp files.
- USE_C_LOG
    define that to use snprintf formatter will be define by default on c files.

- ACTIVE_LOG_LEVEL
    the log level to print from 0(trace to 6(none) see logLevels enum in logLevels.h) default to LOG_TRACE_LEVEL


##todo:
doc all macros and functions.
clean logger.c
add support for other opreation systems?
add support for more log sinks other then stdout.
