/**
 * File:         src/smtp.c
 * Description:  SMTP protocol helper function.
 * Author:       Tomasz Pieczerak (tphaster)
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "smtp-lib.h"
#include "system.h"

int smtp_send_command (int sockfd, size_t cmd, struct mail_object *mail)
{
    char domain[DOMAIN_MAXLEN], cmd_line[LINE_MAXLEN];
    size_t i;
    size_t pos = 0;

    switch (cmd) {
        case EHLO:
        case HELO:
            /* command code */
            if (EHLO == cmd)
                strcpy(cmd_line, "EHLO ");
            else
                strcpy(cmd_line, "HELO ");
            pos = 5;

            /* hostname */
            bzero(domain, sizeof domain);
            gethostname(domain, sizeof domain);
            strncpy(cmd_line+pos, domain, DOMAIN_MAXLEN);
            pos += min(DOMAIN_MAXLEN, strlen(domain));

            /* terminating CRLF */
            strcpy(cmd_line+pos, "\r\n");

            Writen(sockfd, cmd_line, strlen(cmd_line));
            break;

        case MAIL:
            if (NULL == mail)
                return -1;  /* NULL pointer dereference */

            /* command code */
            strcpy(cmd_line, "MAIL FROM:<");
            pos = 11;

            /* sender (Reverse-path) */
            strncpy(cmd_line+pos, mail->mail_from, ADDR_MAXLEN);
            pos += min(ADDR_MAXLEN, strlen(mail->mail_from));

            /* terminating CRLF */
            strcpy(cmd_line+pos, ">\r\n");

            Writen(sockfd, cmd_line, strlen(cmd_line));
            break;

        case RCPT:
            if (NULL == mail)
                return -1;  /* NULL pointer dereference*/

            for (i = 0; i < mail->no_rcpt; ++i) {
                /* command code */
                strcpy(cmd_line, "RCPT TO:<");
                pos = 9;

                /* sender (Reverse-path) */
                strncpy(cmd_line+pos, mail->rcpt_to[i], ADDR_MAXLEN);
                pos += min(ADDR_MAXLEN, strlen(mail->rcpt_to[i]));

                /* terminating CRLF */
                strcpy(cmd_line+pos, ">\r\n");

                Writen(sockfd, cmd_line, strlen(cmd_line));
            }
            break;

        case DATA:
            /* command code and terminating CRLF */
            strcpy(cmd_line, "DATA\r\n");

            Writen(sockfd, cmd_line, strlen(cmd_line));
            break;

        case RSET:
            /* command code and terminating CRLF */
            strcpy(cmd_line, "RSET\r\n");

            Writen(sockfd, cmd_line, strlen(cmd_line));
            break;

        case VRFY:
            return -1;  /* not implemented */
            break;

        case NOOP:
            /* command code and terminating CRLF */
            strcpy(cmd_line, "NOOP\r\n");

            Writen(sockfd, cmd_line, strlen(cmd_line));
            break;

        case QUIT:
            /* command code and terminating CRLF */
            strcpy(cmd_line, "QUIT\r\n");

            Writen(sockfd, cmd_line, strlen(cmd_line));
            break;

        default:
            return -1;  /* no such command */
            break;
    }

    return 0;
}

int smtp_send_reply (int sockfd, size_t code, const char *msg, size_t msg_len)
{
    char domain[DOMAIN_MAXLEN], rply_line[LINE_MAXLEN];
    size_t pos = 0;

    switch (code) {
        case R211:  /* (after QUIT) closing connection */
            if (NULL == msg) {
                /* reply code, text string and terminating <CRLF> */
                strcpy(rply_line, "211 closing connection, bye\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "211 ");
                /* text string is user defined */
                break;
            }

        case R220:  /* (on connection start) greeting */
            if (NULL == msg) {
                /* reply code */
                strcpy(rply_line, "220 ");
                pos = 4;

                /* hostname */
                bzero(domain, sizeof domain);
                gethostname(domain, sizeof domain);
                strncpy(rply_line+pos, domain, DOMAIN_MAXLEN);
                pos += min(DOMAIN_MAXLEN, strlen(domain));

                /* text string and terminating CRLF */
                strcpy(rply_line+pos, " Service ready\r\n");

                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "220 ");
                break;
            }

        case R250:  /* requested mail action okay, completed */
            if (NULL == msg) {
                strcpy(rply_line, "250 OK\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "250 ");
                break;
            }

        case R250E:  /* same as R250, but to be continued */
            if (NULL == msg) {
                return -1;  /* message should be user defined */
            }
            else {
                strcpy(rply_line, "250-");
                break;
            }

        case R251:  /* (after RCPT) user not local; will forward to... */
            /* FIXME: <forward-path> should point to next server */
            if (NULL == msg) {
                strcpy(rply_line, "251 User not local; "
                                  "will forward to <forward-path>\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "251 ");
                break;
            }

        case R354:  /* (after DATA) start mail input; end with <CRLF>.<CRLF> */
            if (NULL == msg) {
                strcpy(rply_line, "354 Start mail input; "
                                  "end with <CRLF>.<CRLF>\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "354 ");
                break;
            }

        case R450:  /* requested mail action not taken: mailbox unavailable */
            if (NULL == msg) {
                strcpy(rply_line, "450 Requested mail action not taken: "
                                  "mailbox unavailable\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "450 ");
                break;
            }

        case R451:  /* requested action aborted: local error in processing */
            if (NULL == msg) {
                strcpy(rply_line, "451 Requested action aborted: "
                                  "local error in processing\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "451 ");
                break;
            }

        case R452:  /* requested action not taken:
                       insufficient system storage */
            if (NULL == msg) {
                strcpy(rply_line, "452 requested action not taken: "
                                  "insufficient system storage\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "452 ");
                break;
            }

        case R455:  /* server unable to accommodate parameters */
            if (NULL == msg) {
                strcpy(rply_line, "455 Server unable to accommodate "
                                  "parameters\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "455  ");
                break;
            }

        case R502:  /* (after EHLO) command not implemented */
            if (NULL == msg) {
                strcpy(rply_line, "502 Command not implemented\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "502 ");
                break;
            }

        case R503:  /* bad sequence of commands */
            if (NULL == msg) {
                strcpy(rply_line, "503 Bad sequence of commands\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "503 ");
                break;
            }

        case R504:  /* (after HELO/EHLO) command parameter not implemented */
            if (NULL == msg) {
                strcpy(rply_line, "504 Command parameter not implemented\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "504 ");
                break;
            }

        case R550:  /* requested action not taken: mailbox unavailable */
            if (NULL == msg) {
                strcpy(rply_line, "550 Requested action not taken: "
                                  "mailbox unavailable\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "550 ");
                break;
            }

        case R551:  /* (after RCPT) user not local; please try <forward-path> */
            /* FIXME: <forward-path> should point to next server */
            if (NULL == msg) {
                strcpy(rply_line, "551 User not local; "
                                  "please try <forward-path>\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "551 ");
                break;
            }

        case R552:  /* requested mail action aborted:
                       exceeded storage allocation */
            if (NULL == msg) {
                strcpy(rply_line, "552 Requested mail action aborted: "
                                  "exceeded storage allocation\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "552 ");
                break;
            }

        case R553:  /* (after RCPT) requested action not taken:
                       mailbox name not allowed */
            if (NULL == msg) {
                strcpy(rply_line, "553 Requested action not taken: "
                                  "mailbox name not allowed\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "553 ");
                break;
            }

        case R554:  /* transaction failed */
            if (NULL == msg) {
                strcpy(rply_line, "554 Transaction failed\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "555 ");
                break;
            }

        case R555:  /* MAIL FROM/RCPT TO parameters not recognized or
                       not implemented */
            if (NULL == msg) {
                strcpy(rply_line, "555 MAIL FROM/RCPT TO parameters not "
                                  "recognized or not implemented\r\n");
                Writen(sockfd, rply_line, strlen(rply_line));
                return 0;
            }
            else {
                strcpy(rply_line, "555 ");
                break;
            }

        default:
            return -1;  /* no such command */
            break;
    }

    /* user defined message */
    pos = 4;
    strncpy(rply_line+pos, msg, min(msg_len,LINE_MAXLEN-pos-3));
    pos += min(LINE_MAXLEN-pos-3, strlen(msg));
    strcpy(rply_line+pos, "\r\n");
    Writen(sockfd, rply_line, strlen(rply_line));

    return 0;
}

/* WARNING! Function buf_read() is not safe for threads */
static ssize_t buf_read (int fd, char *ptr)
{
    static int read_cnt = 0;
    static char *read_ptr;
    static char read_buf[LINE_MAXLEN];

    /* if buffer is empty */
    if (read_cnt <= 0) {
again:
        /* put available data into buffer */
        if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
            if (errno == EINTR)
                goto again;
            return -1;  /* reading error, check errno for more information */
        }
        else if (read_cnt == 0)
            return 0;   /* there is nothing more to read */

        read_ptr = read_buf;
    }

    read_cnt--;
    *ptr = *read_ptr++;

    return 1;   /* one character read */
}

ssize_t smtp_readline (int fd, void *vptr, size_t maxlen)
{
    int rc, state;
    size_t n;
    char c, *ptr;

    ptr = vptr;
    state = START;

    for (n = 1; n < maxlen; n++) {
        if ( (rc = buf_read(fd, &c)) == 1) {
            *ptr++ = c;

            if (CR_READ == state) {
                if (c == '\n') {
                    ptr -= 2;   /* CRLF is not stored */
                    break;
                }
                else
                    state = START;
            }

            if (c == '\r')
                state = CR_READ;    /* CR read; in next turn check for LF */
        }
        else if (rc == 0) {
            if (n == 1)
                return 0;   /* EOF, no data read */
            else
                break;      /* EOF, some data was read */
        }
        else
            return -1;      /* error, errno set by read() */
    }

    *ptr = 0;   /* null terminate like fgets() */

    return n;
}

