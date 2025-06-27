/* APIs for displaying QSO related information */
#ifndef QSO_DISPLAY_H_
#define QSO_DISPLAY_H_

// Forward declaraion
typedef struct Decode Decode;

void display_messages(Decode new_decoded[], int num_decoded);
void display_queued_message(const char*);
void display_txing_message(const char*);
void display_qso_state(const char *txt);

#endif