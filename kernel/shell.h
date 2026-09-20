#ifndef SHELL_H
#define SHELL_H

// Run the interactive shell (never returns). Requires the VFS to be mounted
// and the scheduler initialized.
void shell_run(void);

#endif