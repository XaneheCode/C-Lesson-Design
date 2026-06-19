#ifndef ASSISTANT_H
#define ASSISTANT_H

void assistant_set_api_key(const char *body, char *response_body);
void assistant_get_status(char *response_body);
int assistant_has_api_key(void);
void assistant_clear_api_key(void);
void assistant_handle_chat(const char *body, char *response_body);
void assistant_confirm_sale(const char *body, char *response_body);

#endif
