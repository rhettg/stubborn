#include <stdbool.h>
#include <stdint.h>

#include "command.h"
#include "ci.h"


bool is_cmd(const char *tok, int tok_len, const char *cmd)
{
  if (tok_len < strlen(cmd))
    return false;

  return strncmp(tok, cmd, tok_len) == 0;
}

uint8_t parse_uint8(const char *tok, int tok_len)
{
  char buf[4] = {0, 0, 0, 0};
  if (sizeof(buf) < tok_len) {
    return 0;
  }

  memcpy(buf, tok, tok_len);

  return atoi(buf);
}

int parse_ci_command(char *s) {
  uint8_t cmd = 0;
  uint8_t cmd_data[CI_MAX_DATA] = {0,0,0,0};

  uint8_t v1, v2;

  char *tok_start[3] = {0, 0, 0};
  int tok_len[3] = {0, 0, 0};

  int tok = 0;
  for(int i = 0; i < strlen(s); i++) {
    if (0 == s[i] || '\n' == s[i]) {
      break;
    }

    if (' ' == s[i]) {
      // A tok_start of zero indicates we haven't yet found the beginning of our token.
      if (0 != tok_start[tok]) {
        tok++;
      }
      continue;
    }

    if (0 == tok_start[tok]) {
      tok_start[tok] = &s[i];
    }

    tok_len[tok]++;
  }

  if (is_cmd(tok_start[0], tok_len[0], "NOOP")) {
    cmd = CI_CMD_NOOP;
  } else if (is_cmd(tok_start[0], tok_len[0], "CLEAR")) {
    cmd = CI_CMD_CLEAR;
  } else if (is_cmd(tok_start[0], tok_len[0], "BOOM")) {
    cmd = CI_CMD_BOOM;
  } else if (is_cmd(tok_start[0], tok_len[0], "FWD")) {
    cmd = CI_CMD_EXT_FWD;
    if (0 != tok_len[1]) {
      v1 = parse_uint8(tok_start[1], tok_len[1]);
      if (v1 > 0 && v1 < 256) {
        cmd_data[0] = v1;
      }
    }
  } else if (is_cmd(tok_start[0], tok_len[0], "FFWD")) {
    cmd = CI_CMD_EXT_FFWD;
    if (0 != tok_len[1]) {
      v1 = parse_uint8(tok_start[1], tok_len[1]);
      if (v1 > 0 && v1 < 256) {
        cmd_data[0] = v1;
      }
    }
  } else if (is_cmd(tok_start[0], tok_len[0], "BCK")) {
    cmd = CI_CMD_EXT_BCK;
    if (0 != tok_len[1]) {
      v1 = parse_uint8(tok_start[1], tok_len[1]);
      if (v1 > 0 && v1 < 256) {
        cmd_data[0] = v1;
      }
    }
  } else if (is_cmd(tok_start[0], tok_len[0], "RT")) {
    cmd = CI_CMD_EXT_RT;
    if (0 != tok_len[1]) {
      v1 = parse_uint8(tok_start[1], tok_len[1]);
      if (v1 > 0 && v1 < 256) {
        cmd_data[0] = v1;
      }
    }
  } else if (is_cmd(tok_start[0], tok_len[0], "LT")) {
    cmd = CI_CMD_EXT_LT;
    if (0 != tok_len[1]) {
      v1 = parse_uint8(tok_start[1], tok_len[1]);
      if (v1 > 0 && v1 < 256) {
        cmd_data[0] = v1;
      }
    }
  } else if (is_cmd(tok_start[0], tok_len[0], "STOP")) {
      cmd_data[0] = 0;
      cmd = CI_CMD_EXT_STOP;
  } else if (is_cmd(tok_start[0], tok_len[0], "SET")) {
    cmd = CI_CMD_EXT_SET;
    if (0 != tok_len[1]) {
      v1 = parse_uint8(tok_start[1], tok_len[1]);
      cmd_data[0] = v1;
    }
    if (0 != tok_len[2]) {
      v2 = parse_uint8(tok_start[2], tok_len[2]);
      cmd_data[3] = v2;
    }
  }

  if (0 == cmd) {
    return ERR_UNKNOWN;
  }

  if (0 != CI_prepare_send(&ci, cmd, cmd_data, millis())) {
    return ERR_CI_SEND_FAIL;
  }

  return 0;
}

#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '

/*************************************************************************************************************
    getCommandLineFromSerialPort()
      Return the string of the next command. Commands are delimited by return"
      Handle BackSpace character
      Make all chars lowercase

    Adapted from https://create.arduino.cc/projecthub/mikefarr/simple-command-line-interface-4f0a3f
*************************************************************************************************************/

int feed_command_data(char *c_buf, size_t *c_ndx, char *data, size_t data_len)
{
  for(int n = 0; n < data_len; n++) {
    switch (data[n]) {
      case CR:      //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        c_buf[c_ndx] = NULLCHAR;
        c_ndx = 0;
        return n;
        break;
      case BS:
        if (c_ndx > 0) {
          c_buf[--c_ndx] = NULLCHAR;
        }
        break;
      default:
        if (c_ndx < MAX_COMMAND_LENGTH) {
          c_buf[c_ndx++] = data[n];
        }
        c_buf[c_ndx] = NULLCHAR;
        break;
    }
  }
  return -1;
}