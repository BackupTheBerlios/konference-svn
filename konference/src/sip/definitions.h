/***************************************************************************
 *   Copyright (C) 2005 by Malte Böhme                                     *
 *   malte.boehme@rwth-aachen.de                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef DEFINES_H
#define DEFINES_H


// Call States
#define SIP_IDLE                0x1
#define SIP_OCONNECTING1        0x2    // Invite sent, no response yet
#define SIP_OCONNECTING2        0x3    // Invite sent, 1xx response
#define SIP_ICONNECTING         0x4
#define SIP_CONNECTED           0x5
#define SIP_DISCONNECTING       0x6
#define SIP_CONNECTED_VXML      0x7    // This is a false state, only used as indication back to the frontend

// Registration States
#define SIP_REG_DISABLED        0x01   // Proxy registration turned off
#define SIP_REG_TRYING          0x02   // Sent a REGISTER, waiting for an answer
#define SIP_REG_CHALLENGED      0x03   // Initial REGISTER was met with a challenge, sent an authorized REGISTER
#define SIP_REG_FAILED          0x04   // REGISTER failed; will retry after a period of time
#define SIP_REG_REGISTERED      0x05   // Registration successful

// Presence Subscriber States
#define SIP_SUB_IDLE            SIP_IDLE
#define SIP_SUB_SUBSCRIBED      0x10

// Presence Watcher States
#define SIP_WATCH_IDLE          SIP_IDLE
#define SIP_WATCH_TRYING        0x20
#define SIP_WATCH_ACTIVE        0x21
#define SIP_WATCH_STOPPING      0x22
#define SIP_WATCH_HOLDOFF       0x23

// IM States
#define SIP_IM_IDLE             SIP_IDLE
#define SIP_IM_ACTIVE           0x30


// Events
#define SIP_UNKNOWN             0x0
#define SIP_OUTCALL             0x100
#define SIP_INVITE              0x200
#define SIP_INVITESTATUS_2xx    0x300
#define SIP_INVITESTATUS_1xx    0x400
#define SIP_INVITESTATUS_3456xx 0x500
#define SIP_ANSWER              0x600
#define SIP_ACK                 0x700
#define SIP_BYE                 0x800
#define SIP_HANGUP              0x900
#define SIP_BYESTATUS           0xA00
#define SIP_CANCEL              0xB00
#define SIP_CANCELSTATUS        0xC00
#define SIP_REGISTER            0xD00
#define SIP_RETX                0xE00
#define SIP_REGISTRAR_TEXP      0xF00
#define SIP_REGSTATUS           0x1000
#define SIP_REG_TREGEXP         0x1100
#define SIP_SUBSCRIBE           0x1200
#define SIP_SUBSTATUS           0x1300
#define SIP_NOTIFY              0x1400
#define SIP_NOTSTATUS           0x1500
#define SIP_PRESENCE_CHANGE     0x1600
#define SIP_SUBSCRIBE_EXPIRE    0x1700
#define SIP_WATCH               0x1800
#define SIP_STOPWATCH           0x1900
#define SIP_MESSAGE             0x1A00
#define SIP_MESSAGESTATUS       0x1B00
#define SIP_INFO                0x1C00
#define SIP_INFOSTATUS          0x1D00
#define SIP_IM_TIMEOUT          0x1E00
#define SIP_USER_MESSAGE        0x1F00
#define SIP_KICKWATCH           0x2000

#define SIP_CMD(s)              (((s)==SIP_INVITE) || ((s)==SIP_ACK) || ((s)==SIP_BYE) || ((s)==SIP_CANCEL) || ((s)==SIP_REGISTER) || ((s)==SIP_SUBSCRIBE) || ((s)==SIP_NOTIFY) || ((s)==SIP_MESSAGE) || ((s)==SIP_INFO))
#define SIP_STATUS(s)           (((s)==SIP_INVITESTATUS_2xx) || ((s)==SIP_INVITESTATUS_1xx) || ((s)==SIP_INVITESTATUS_3456xx) || ((s)==SIP_BYTESTATUS) || ((s)==SIP_CANCELSTATUS) || ((s)==SIP_SUBSTATUS) || ((s)==SIP_NOTSTATUS) || ((s)==SIP_MESSAGESTATUS) || ((s)==SIP_INFOSTATUS) )
#define SIP_MSG(s)              (SIP_CMD(s) || SIP_STATUS(s))

// Call FSM Actions - combination of event and state to give a "switch"able value
#define SIP_IDLE_OUTCALL                  (SIP_IDLE          | SIP_OUTCALL)
#define SIP_IDLE_BYE                      (SIP_IDLE          | SIP_BYE)
#define SIP_IDLE_INVITE                   (SIP_IDLE          | SIP_INVITE)
#define SIP_IDLE_INVITESTATUS_1xx         (SIP_IDLE          | SIP_INVITESTATUS_1xx)
#define SIP_IDLE_INVITESTATUS_2xx         (SIP_IDLE          | SIP_INVITESTATUS_2xx)
#define SIP_IDLE_INVITESTATUS_3456        (SIP_IDLE          | SIP_INVITESTATUS_3456xx)
#define SIP_OCONNECTING1_INVITESTATUS_3456 (SIP_OCONNECTING1  | SIP_INVITESTATUS_3456xx)
#define SIP_OCONNECTING1_INVITESTATUS_2xx (SIP_OCONNECTING1  | SIP_INVITESTATUS_2xx)
#define SIP_OCONNECTING1_INVITESTATUS_1xx (SIP_OCONNECTING1  | SIP_INVITESTATUS_1xx)
#define SIP_OCONNECTING1_RETX             (SIP_OCONNECTING1  | SIP_RETX)
#define SIP_OCONNECTING2_INVITESTATUS_3456 (SIP_OCONNECTING2  | SIP_INVITESTATUS_3456xx)
#define SIP_OCONNECTING2_INVITESTATUS_2xx (SIP_OCONNECTING2  | SIP_INVITESTATUS_2xx)
#define SIP_OCONNECTING2_INVITESTATUS_1xx (SIP_OCONNECTING2  | SIP_INVITESTATUS_1xx)
#define SIP_OCONNECTING1_HANGUP           (SIP_OCONNECTING1  | SIP_HANGUP)
#define SIP_OCONNECTING2_HANGUP           (SIP_OCONNECTING2  | SIP_HANGUP)
#define SIP_OCONNECTING1_INVITE           (SIP_OCONNECTING1  | SIP_INVITE)
#define SIP_ICONNECTING_INVITE            (SIP_ICONNECTING   | SIP_INVITE)
#define SIP_ICONNECTING_ANSWER            (SIP_ICONNECTING   | SIP_ANSWER)
#define SIP_ICONNECTING_CANCEL            (SIP_ICONNECTING   | SIP_CANCEL)
#define SIP_CONNECTED_ACK                 (SIP_CONNECTED     | SIP_ACK)
#define SIP_CONNECTED_INVITESTATUS_2xx    (SIP_CONNECTED     | SIP_INVITESTATUS_2xx)
#define SIP_CONNECTED_RETX                (SIP_CONNECTED     | SIP_RETX)
#define SIP_CONNECTED_BYE                 (SIP_CONNECTED     | SIP_BYE)
#define SIP_CONNECTED_HANGUP              (SIP_CONNECTED     | SIP_HANGUP)
#define SIP_DISCONNECTING_BYESTATUS       (SIP_DISCONNECTING | SIP_BYESTATUS)
#define SIP_DISCONNECTING_ACK             (SIP_DISCONNECTING | SIP_ACK)
#define SIP_DISCONNECTING_RETX            (SIP_DISCONNECTING | SIP_RETX)
#define SIP_DISCONNECTING_CANCEL          (SIP_DISCONNECTING | SIP_CANCEL)
#define SIP_DISCONNECTING_CANCELSTATUS    (SIP_DISCONNECTING | SIP_CANCELSTATUS)
#define SIP_DISCONNECTING_BYE             (SIP_DISCONNECTING | SIP_BYE)

// Registration FSM Actions - combination of event and state to give a "switch"able value
#define SIP_REG_TRYING_STATUS             (SIP_REG_TRYING    | SIP_REGSTATUS)
#define SIP_REG_CHALL_STATUS              (SIP_REG_CHALLENGED| SIP_REGSTATUS)
#define SIP_REG_REGISTERED_TREGEXP        (SIP_REG_REGISTERED| SIP_REG_TREGEXP)
#define SIP_REG_TRYING_RETX               (SIP_REG_TRYING    | SIP_RETX)
#define SIP_REG_CHALL_RETX                (SIP_REG_CHALLENGED| SIP_RETX)
#define SIP_REG_FAILED_RETX               (SIP_REG_FAILED    | SIP_RETX)

// Presence Subscriber FSM Actions - combination of event and state to give a "switch"able value
#define SIP_SUB_IDLE_SUBSCRIBE            (SIP_SUB_IDLE       | SIP_SUBSCRIBE)
#define SIP_SUB_SUBS_SUBSCRIBE            (SIP_SUB_SUBSCRIBED | SIP_SUBSCRIBE)
#define SIP_SUB_SUBS_SUBSCRIBE_EXPIRE     (SIP_SUB_SUBSCRIBED | SIP_SUBSCRIBE_EXPIRE)
#define SIP_SUB_SUBS_RETX                 (SIP_SUB_SUBSCRIBED | SIP_RETX)
#define SIP_SUB_SUBS_NOTSTATUS            (SIP_SUB_SUBSCRIBED | SIP_NOTSTATUS)
#define SIP_SUB_SUBS_PRESENCE_CHANGE      (SIP_SUB_SUBSCRIBED | SIP_PRESENCE_CHANGE)

// Presence Watcher FSM Actions - combination of event and state to give a "switch"able value
#define SIP_WATCH_IDLE_WATCH              (SIP_WATCH_IDLE     | SIP_WATCH)
#define SIP_WATCH_TRYING_WATCH            (SIP_WATCH_TRYING   | SIP_WATCH)
#define SIP_WATCH_ACTIVE_SUBSCRIBE_EXPIRE (SIP_WATCH_ACTIVE   | SIP_SUBSCRIBE_EXPIRE)
#define SIP_WATCH_TRYING_RETX             (SIP_WATCH_TRYING   | SIP_RETX)
#define SIP_WATCH_ACTIVE_RETX             (SIP_WATCH_ACTIVE   | SIP_RETX)
#define SIP_WATCH_TRYING_SUBSTATUS        (SIP_WATCH_TRYING   | SIP_SUBSTATUS)
#define SIP_WATCH_ACTIVE_SUBSTATUS        (SIP_WATCH_ACTIVE   | SIP_SUBSTATUS)
#define SIP_WATCH_ACTIVE_NOTIFY           (SIP_WATCH_ACTIVE   | SIP_NOTIFY)
#define SIP_WATCH_TRYING_STOPWATCH        (SIP_WATCH_TRYING   | SIP_STOPWATCH)
#define SIP_WATCH_ACTIVE_STOPWATCH        (SIP_WATCH_ACTIVE   | SIP_STOPWATCH)
#define SIP_WATCH_STOPPING_RETX           (SIP_WATCH_STOPPING | SIP_RETX)
#define SIP_WATCH_STOPPING_SUBSTATUS      (SIP_WATCH_STOPPING | SIP_SUBSTATUS)
#define SIP_WATCH_TRYING_SUBSCRIBE        (SIP_WATCH_TRYING   | SIP_SUBSCRIBE)
#define SIP_WATCH_HOLDOFF_WATCH           (SIP_WATCH_HOLDOFF  | SIP_WATCH)
#define SIP_WATCH_HOLDOFF_STOPWATCH       (SIP_WATCH_HOLDOFF  | SIP_STOPWATCH)
#define SIP_WATCH_HOLDOFF_SUBSCRIBE       (SIP_WATCH_HOLDOFF  | SIP_SUBSCRIBE)
#define SIP_WATCH_HOLDOFF_KICK            (SIP_WATCH_HOLDOFF  | SIP_KICKWATCH)


// Build Options logically OR'ed and sent to build procs
#define SIP_OPT_SDP		1
#define SIP_OPT_CONTACT	2
#define SIP_OPT_VIA		4
#define SIP_OPT_ALLOW	8
#define SIP_OPT_EXPIRES	16

// Timers
#define REG_RETRY_TIMER			3000 // seconds
#define REG_FAIL_RETRY_TIMER	180000 // 3 minutes
#define REG_RETRY_MAXCOUNT		5

#define SIP_POLL_PERIOD			2   // Twice per second


#endif //DEFINES_H
