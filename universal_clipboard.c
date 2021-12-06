#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<yed/plugin.h>

void copy_to_universal_clipboard(int argc, char **argv);
void cmd_pre_handler(yed_event *event);
void var_post_change(yed_event *event);

static int universal_copy_on_yank_state;
static yed_event_handler copy_yank_pre;
static yed_plugin *global_self;

static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//code modified from stack overflow post to get the b4 bit encoded verison of a char array
char *b64_encode(const char *in, size_t len)
{
    char   *out;
    size_t  elen;
    size_t  i;
    size_t  j;
    size_t  v;

    if (in == NULL || len == 0)
        return NULL;

    elen = len;
    if (len % 3 != 0)
        elen += 3 - (len % 3);
    elen /= 3;
    elen *= 4;
    out  = malloc(elen+1);
    out[elen] = '\0';

    for (i=0, j=0; i<len; i+=3, j+=4) {
        v = in[i];
        v = i+1 < len ? v << 8 | in[i+1] : v << 8;
        v = i+2 < len ? v << 8 | in[i+2] : v << 8;

        out[j]   = b64chars[(v >> 18) & 0x3F];
        out[j+1] = b64chars[(v >> 12) & 0x3F];
        if (i+1 < len) {
            out[j+2] = b64chars[(v >> 6) & 0x3F];
        } else {
            out[j+2] = '=';
        }
        if (i+2 < len) {
            out[j+3] = b64chars[v & 0x3F];
        } else {
            out[j+3] = '=';
        }
    }

    return out;
}

int yed_plugin_boot(yed_plugin *self){
    char * copy_on_yank;
    yed_event_handler variable_change_event;

    YED_PLUG_VERSION_CHECK();

    yed_plugin_set_command(self, "copy-to-universal-clipboard", copy_to_universal_clipboard);

    variable_change_event.kind = EVENT_VAR_POST_SET;
    variable_change_event.fn = var_post_change;

    copy_yank_pre.kind = EVENT_CMD_PRE_RUN;
    copy_yank_pre.fn = cmd_pre_handler;

    global_self = self;

    copy_on_yank = yed_get_var("universal-copy-on-yank");

    yed_plugin_add_event_handler(global_self, variable_change_event);

    if(copy_on_yank == NULL){
        yed_set_var("universal-copy-on-yank","NO");
    }else if(yed_var_is_truthy("universal-copy-on-yank")){
        yed_plugin_add_event_handler(global_self, copy_yank_pre);
        universal_copy_on_yank_state=1;
    }else{
        universal_copy_on_yank_state=0;
    }

    return 0;
}


void copy_to_universal_clipboard(int argc, char **argv){
    char * selection;
    char * base64;
    char * out_buffer;
    yed_buffer *buffer = ys->active_frame!=NULL?ys->active_frame->buffer:NULL;
    if(buffer !=NULL && buffer->has_selection){
        selection = yed_get_selection_text(buffer);
        if(selection != NULL && strcmp(selection,"")!=0){
            base64 = b64_encode(selection,strlen(selection));
            out_buffer = malloc(strlen(base64)+20);
            snprintf(out_buffer,strlen(base64)+20,"\033]52;c;%s\a",base64);
            append_to_output_buff(out_buffer);
            yed_cprint("Copied to system clipboard");
            free(out_buffer);
            free(selection);
            free(base64);
        }
    }
}

void cmd_pre_handler(yed_event *event){
    if(strcmp(event->cmd_name,"yank-selection")==0){
        copy_to_universal_clipboard(0,NULL);
    }
}


void var_post_change(yed_event *event){
    int state = yed_var_is_truthy("universal-copy-on-yank");
    if(state != universal_copy_on_yank_state){
        if(state){
            yed_plugin_add_event_handler(global_self,copy_yank_pre);
        }else{
            yed_delete_event_handler(copy_yank_pre);
        }
        universal_copy_on_yank_state=state;
    }
}
