#ifndef FMI_METRICS_H
#define FMI_METRICS_H

#define FMI_REMOTE_RECORD_EXEC_START(name) record_exec_start(FmiFunctionNames::name);
#define FMI_REMOTE_RECORD_EXEC_END(name,start) record_exec_end(FmiFunctionNames::name, start);

#endif // FMI_METRICS_H
