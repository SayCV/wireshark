/* packet-wsp.c
 *
 * Routines to dissect WSP component of WAP traffic.
 * 
 * $Id: packet-wsp.c,v 1.24 2001/07/20 04:39:07 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Didier Jorand
 *
 * WAP dissector based on original work by Ben Fowler
 * Updated by Neil Hunter <neil.hunter@energis-squared.com>
 * WTLS support by Alexandre P. Ferreira (Splice IP)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef NEED_SNPRINTF_H
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
# include "snprintf.h"
#endif

#include <string.h>
#include <glib.h>
#include "packet.h"
#include "conversation.h"
#include "packet-wap.h"
#include "packet-wsp.h"

/* File scoped variables for the protocol and registered fields */
static int proto_wsp 							= HF_EMPTY;

/* These fields used by fixed part of header */
static int hf_wsp_header_tid					= HF_EMPTY;
static int hf_wsp_header_pdu_type				= HF_EMPTY;
static int hf_wsp_version_major					= HF_EMPTY;
static int hf_wsp_version_minor					= HF_EMPTY;
static int hf_wsp_capability_length				= HF_EMPTY;
static int hf_wsp_capabilities_section				= HF_EMPTY;
static int hf_wsp_capabilities_client_SDU			= HF_EMPTY;
static int hf_wsp_capabilities_server_SDU			= HF_EMPTY;
static int hf_wsp_capabilities_protocol_opt			= HF_EMPTY;
static int hf_wsp_capabilities_method_MOR			= HF_EMPTY;
static int hf_wsp_capabilities_push_MOR				= HF_EMPTY;
static int hf_wsp_capabilities_extended_methods			= HF_EMPTY;
static int hf_wsp_capabilities_header_code_pages		= HF_EMPTY;
static int hf_wsp_capabilities_aliases				= HF_EMPTY;
static int hf_wsp_header_uri_len				= HF_EMPTY;
static int hf_wsp_header_uri					= HF_EMPTY;
static int hf_wsp_server_session_id				= HF_EMPTY;
static int hf_wsp_header_status					= HF_EMPTY;
static int hf_wsp_header_length					= HF_EMPTY;
static int hf_wsp_headers_section				= HF_EMPTY;
static int hf_wsp_header						= HF_EMPTY;
static int hf_wsp_content_type					= HF_EMPTY;
static int hf_wsp_parameter_well_known_charset	= HF_EMPTY;
static int hf_wsp_reply_data					= HF_EMPTY;
static int hf_wsp_post_data						= HF_EMPTY;

static int hf_wsp_header_shift_code					= HF_EMPTY;
static int hf_wsp_header_accept					= HF_EMPTY;
static int hf_wsp_header_accept_str				= HF_EMPTY;
static int hf_wsp_header_accept_charset			= HF_EMPTY;
static int hf_wsp_header_accept_charset_str		= HF_EMPTY;
static int hf_wsp_header_accept_language		= HF_EMPTY;
static int hf_wsp_header_accept_language_str	= HF_EMPTY;
static int hf_wsp_header_accept_ranges			= HF_EMPTY;
static int hf_wsp_header_cache_control			= HF_EMPTY;
static int hf_wsp_header_content_length			= HF_EMPTY;
static int hf_wsp_header_age					= HF_EMPTY;
static int hf_wsp_header_date					= HF_EMPTY;
static int hf_wsp_header_etag					= HF_EMPTY;
static int hf_wsp_header_expires				= HF_EMPTY;
static int hf_wsp_header_last_modified			= HF_EMPTY;
static int hf_wsp_header_location				= HF_EMPTY;
static int hf_wsp_header_if_modified_since		= HF_EMPTY;
static int hf_wsp_header_server					= HF_EMPTY;
static int hf_wsp_header_user_agent				= HF_EMPTY;
static int hf_wsp_header_application_header		= HF_EMPTY;
static int hf_wsp_header_application_value		= HF_EMPTY;
static int hf_wsp_header_x_wap_tod				= HF_EMPTY;
static int hf_wsp_header_transfer_encoding		= HF_EMPTY;
static int hf_wsp_header_transfer_encoding_str	= HF_EMPTY;
static int hf_wsp_header_via					= HF_EMPTY;

static int hf_wsp_redirect_flags			= HF_EMPTY;
static int hf_wsp_redirect_permanent			= HF_EMPTY;
static int hf_wsp_redirect_reuse_security_session	= HF_EMPTY;
static int hf_wsp_redirect_afl				= HF_EMPTY;
static int hf_wsp_redirect_afl_bearer_type_included	= HF_EMPTY;
static int hf_wsp_redirect_afl_port_number_included	= HF_EMPTY;
static int hf_wsp_redirect_afl_address_len		= HF_EMPTY;
static int hf_wsp_redirect_bearer_type			= HF_EMPTY;
static int hf_wsp_redirect_port_num			= HF_EMPTY;
static int hf_wsp_redirect_ipv4_addr			= HF_EMPTY;
static int hf_wsp_redirect_addr				= HF_EMPTY;

/* Initialize the subtree pointers */
static gint ett_wsp 							= ETT_EMPTY;
static gint ett_header 							= ETT_EMPTY;
static gint ett_headers							= ETT_EMPTY;
static gint ett_capabilities					= ETT_EMPTY;
static gint ett_content_type					= ETT_EMPTY;
static gint ett_redirect_flags				= ETT_EMPTY;
static gint ett_redirect_afl				= ETT_EMPTY;

/* Handle for WMLC dissector */
static dissector_handle_t wmlc_handle;

static const value_string vals_pdu_type[] = {
	{ 0x00, "Reserved" },
	{ 0x01, "Connect" },
	{ 0x02, "ConnectReply" },
	{ 0x03, "Redirect" },
	{ 0x04, "Reply" },
	{ 0x05, "Disconnect" },
	{ 0x06, "Push" },
	{ 0x07, "ConfirmedPush" },
	{ 0x08, "Suspend" },
	{ 0x09, "Resume" },

	/* 0x10 - 0x3F Unassigned */

	{ 0x40, "Get" },
	{ 0x41, "Options" },
	{ 0x42, "Head" },
	{ 0x43, "Delete" },
	{ 0x44, "Trace" },

	/* 0x45 - 0x4F Unassigned (Get PDU) */
	/* 0x50 - 0x5F Extended method (Get PDU) */

	{ 0x60, "Post" },
	{ 0x61, "Put" },

	/* 0x62 - 0x6F Unassigned (Post PDU) */
	/* 0x70 - 0x7F Extended method (Post PDU) */
	/* 0x80 - 0xFF Reserved */

	{ 0x00, NULL }

};

static const value_string vals_status[] = {
	/* 0x00 - 0x0F Reserved */

	{ 0x10, "Continue" },
	{ 0x11, "Switching Protocols" },

	{ 0x20, "OK" },
	{ 0x21, "Created" },
	{ 0x22, "Accepted" },
	{ 0x23, "Non-Authoritative Information" },
	{ 0x24, "No Content" },
	{ 0x25, "Reset Content" },
	{ 0x26, "Partial Content" },

	{ 0x30, "Multiple Choices" },
	{ 0x31, "Moved Permanently" },
	{ 0x32, "Moved Temporarily" },
	{ 0x33, "See Other" },
	{ 0x34, "Not Modified" },
	{ 0x35, "Use Proxy" },

	{ 0x40, "Bad Request" },
	{ 0x41, "Unauthorised" },
	{ 0x42, "Payment Required" },
	{ 0x43, "Forbidden" },
	{ 0x44, "Not Found" },
	{ 0x45, "Method Not Allowed" },
	{ 0x46, "Not Acceptable" },
	{ 0x47, "Proxy Authentication Required" },
	{ 0x48, "Request Timeout" },
	{ 0x49, "Conflict" },
	{ 0x4A, "Gone" },
	{ 0x4B, "Length Required" },
	{ 0x4C, "Precondition Failed" },
	{ 0x4D, "Request Entity Too Large" },
	{ 0x4E, "Request-URI Too Large" },
	{ 0x4F, "Unsupported Media Type" },

	{ 0x60, "Internal Server Error" },
	{ 0x61, "Not Implemented" },
	{ 0x62, "Bad Gateway" },
	{ 0x63, "Service Unavailable" },
	{ 0x64, "Gateway Timeout" },
	{ 0x65, "HTTP Version Not Supported" },
	{ 0x00, NULL }
};

static const value_string vals_content_types[] = {
	{ 0x00, "*/*" },
	{ 0x01, "text/*" },
	{ 0x02, "text/html" },
	{ 0x03, "text/plain" },
	{ 0x04, "text/x-hdml" },
	{ 0x05, "text/x-ttml" },
	{ 0x06, "text/x-vCalendar" },
	{ 0x07, "text/x-vCard" },
	{ 0x08, "text/vnd.wap.wml" },
	{ 0x09, "text/vnd.wap.wmlscript" },
	{ 0x0A, "text/vnd.wap.channel" },
	{ 0x0B, "Multipart/*" },
	{ 0x0C, "Multipart/mixed" },
	{ 0x0D, "Multipart/form-data" },
	{ 0x0E, "Multipart/byteranges" },
	{ 0x0F, "Multipart/alternative" },
	{ 0x10, "application/*" },
	{ 0x11, "application/java-vm" },
	{ 0x12, "application/x-www-form-urlencoded" },
	{ 0x13, "application/x-hdmlc" },
	{ 0x14, "application/vnd.wap.wmlc" },
	{ 0x15, "application/vnd.wap.wmlscriptc" },
	{ 0x16, "application/vnd.wap.channelc" },
	{ 0x17, "application/vnd.wap.uaprof" },
	{ 0x18, "application/vnd.wap.wtls-ca-certificate" },
	{ 0x19, "application/vnd.wap.wtls-user-certificate" },
	{ 0x1A, "application/x-x509-ca-cert" },
	{ 0x1B, "application/x-x509-user-cert" },
	{ 0x1C, "image/*" },
	{ 0x1D, "image/gif" },
	{ 0x1E, "image/jpeg" },
	{ 0x1F, "image/tiff" },
	{ 0x20, "image/png" },
	{ 0x21, "image/vnd.wap.wbmp" },
	{ 0x22, "application/vnd.wap.multipart.*" },
	{ 0x23, "application/vnd.wap.multipart.mixed" },
	{ 0x24, "application/vnd.wap.multipart.form-data" },
	{ 0x25, "application/vnd.wap.multipart.byteranges" },
	{ 0x26, "application/vnd.wap.multipart.alternative" },
	{ 0x27, "application/xml" },
	{ 0x28, "text/xml" },
	{ 0x29, "application/vnd.wap.wbxml" },
	{ 0x2A, "application/x-x968-cross-cert" },
	{ 0x2B, "application/x-x968-ca-cert" },
	{ 0x2C, "application/x-x968-user-cert" },
	{ 0x2D, "text/vnd.wap.si" },
	{ 0x2E, "application/vnd.wap.sic" },
	{ 0x2F, "text/vnd.wap.sl" },
	{ 0x30, "application/vnd.wap.slc" },
	{ 0x31, "text/vnd.wap.co" },
	{ 0x32, "application/vnd.wap.coc" },
	{ 0x33, "application/vnd.wap.multipart.related" },
	{ 0x34, "application/vnd.wap.sia" },
	{ 0x00, NULL }
};

static const value_string vals_languages[] = {
	{ 0x01, "Afar (aa)" },
	{ 0x02, "Abkhazian (ab)" },
	{ 0x03, "Afrikaans (af)" },
	{ 0x04, "Amharic (am)" },
	{ 0x05, "Arabic (ar)" },
	{ 0x06, "Assamese (as)" },
	{ 0x07, "Aymara (ay)" },
	{ 0x08, "Azerbaijani (az)" },
	{ 0x09, "Bashkir (ba)" },
	{ 0x0A, "Byelorussian (be)" },
	{ 0x0B, "Bulgarian (bg)" },
	{ 0x0C, "Bihari (bh)" },
	{ 0x0D, "Bislama (bi)" },
	{ 0x0E, "Bengali; Bangla (bn)" },
	{ 0x0F, "Tibetan (bo)" },
	{ 0x10, "Breton (br)" },
	{ 0x11, "Catalan (ca)" },
	{ 0x12, "Corsican (co)" },
	{ 0x13, "Czech (cs)" },
	{ 0x14, "Welsh (cy)" },
	{ 0x15, "Danish (da)" },
	{ 0x16, "German (de)" },
	{ 0x17, "Bhutani (dz)" },
	{ 0x18, "Greek (el)" },
	{ 0x19, "English (en)" },
	{ 0x1A, "Esperanto (eo)" },
	{ 0x1B, "Spanish (es)" },
	{ 0x1C, "Estonian (et)" },
	{ 0x1D, "Basque (eu)" },
	{ 0x1E, "Persian (fa)" },
	{ 0x1F, "Finnish (fi)" },
	{ 0x20, "Fiji (fj)" },
	{ 0x22, "French (fr)" },
	{ 0x24, "Irish (ga)" },
	{ 0x25, "Scots Gaelic (gd)" },
	{ 0x26, "Galician (gl)" },
	{ 0x27, "Guarani (gn)" },
	{ 0x28, "Gujarati (gu)" },
	{ 0x29, "Hausa (ha)" },
	{ 0x2A, "Hebrew (formerly iw) (he)" },
	{ 0x2B, "Hindi (hi)" },
	{ 0x2C, "Croatian (hr)" },
	{ 0x2D, "Hungarian (hu)" },
	{ 0x2E, "Armenian (hy)" },
	{ 0x30, "Indonesian (formerly in) (id)" },
	{ 0x47, "Maori (mi)" },
	{ 0x48, "Macedonian (mk)" },
	{ 0x49, "Malayalam (ml)" },
	{ 0x4A, "Mongolian (mn)" },
	{ 0x4B, "Moldavian (mo)" },
	{ 0x4C, "Marathi (mr)" },
	{ 0x4D, "Malay (ms)" },
	{ 0x4E, "Maltese (mt)" },
	{ 0x4F, "Burmese (my)" },
	{ 0x51, "Nepali (ne)" },
	{ 0x52, "Dutch (nl)" },
	{ 0x53, "Norwegian (no)" },
	{ 0x54, "Occitan (oc)" },
	{ 0x55, "(Afan) Oromo (om)" },
	{ 0x56, "Oriya (or)" },
	{ 0x57, "Punjabi (pa)" },
	{ 0x58, "Polish (po)" },
	{ 0x59, "Pashto, Pushto (ps)" },
	{ 0x5A, "Portuguese (pt)" },
	{ 0x5B, "Quechua (qu)" },
	{ 0x5D, "Kirundi (rn)" },
	{ 0x5E, "Romanian (ro)" },
	{ 0x5F, "Russian (ru)" },
	{ 0x60, "Kinyarwanda (rw)" },
	{ 0x61, "Sanskrit (sa)" },
	{ 0x62, "Sindhi (sd)" },
	{ 0x63, "Sangho (sg)" },
	{ 0x64, "Serbo-Croatian (sh)" },
	{ 0x65, "Sinhalese (si)" },
	{ 0x66, "Slovak (sk)" },
	{ 0x67, "Slovenian (sl)" },
	{ 0x68, "Samoan (sm)" },
	{ 0x69, "Shona (sn)" },
	{ 0x6A, "Somali (so)" },
	{ 0x6B, "Albanian (sq)" },
	{ 0x6C, "Serbian (sr)" },
	{ 0x6D, "Siswati (ss)" },
	{ 0x6E, "Sesotho (st)" },
	{ 0x6F, "Sundanese (su)" },
	{ 0x70, "Swedish (sv)" },
	{ 0x71, "Swahili (sw)" },
	{ 0x72, "Tamil (ta)" },
	{ 0x73, "Telugu (te)" },
	{ 0x74, "Tajik (tg)" },
	{ 0x75, "Thai (th)" },
	{ 0x76, "Tigrinya (ti)" },
	{ 0x81, "Nauru (na)" },
	{ 0x82, "Faeroese (fo)" },
	{ 0x83, "Frisian (fy)" },
	{ 0x84, "Interlingua (ia)" },
	{ 0x8C, "Rhaeto-Romance (rm)" },
	{ 0x00, NULL }
};

static const value_string vals_accept_ranges[] = {
	{ 0x80, "None" },
	{ 0x81, "Bytes" },
	{ 0x00, NULL }
};

static const value_string vals_cache_control[] = {
	{ 0x80, "No-cache" },
	{ 0x81, "No-store" },
	{ 0x82, "Max-age" },
	{ 0x83, "Max-stale" },
	{ 0x84, "Min-fresh" },
	{ 0x85, "Only-if-cached" },
	{ 0x86, "Public" },
	{ 0x87, "Private" },
	{ 0x88, "No-transform" },
	{ 0x89, "Must-revalidate" },
	{ 0x8A, "Proxy-revalidate" },
	{ 0x00, NULL }
};

static const value_string vals_transfer_encoding[] = {
	{ 0x80, "Chunked" },
	{ 0x00, NULL }
};

/*
 * Redirect flags.
 */
#define PERMANENT_REDIRECT	0x80
#define REUSE_SECURITY_SESSION	0x40

/*
 * Redirect address flags and length.
 */
#define BEARER_TYPE_INCLUDED	0x80
#define PORT_NUMBER_INCLUDED	0x40
#define ADDRESS_LEN		0x3f

static const true_false_string yes_no_truth = { 
	"Yes" ,
	"No"
};

/*
 * Windows appears to define DELETE.
 */
#ifdef DELETE
#undef DELETE
#endif

enum {
	RESERVED		= 0x00,
	CONNECT			= 0x01,
	CONNECTREPLY	= 0x02,
	REDIRECT		= 0x03,			/* No sample data */
	REPLY			= 0x04,
	DISCONNECT		= 0x05,
	PUSH			= 0x06,			/* No sample data */
	CONFIRMEDPUSH	= 0x07,			/* No sample data */
	SUSPEND			= 0x08,			/* No sample data */
	RESUME			= 0x09,			/* No sample data */

	GET				= 0x40,
	OPTIONS			= 0x41,			/* No sample data */
	HEAD			= 0x42,			/* No sample data */
	DELETE			= 0x43,			/* No sample data */
	TRACE			= 0x44,			/* No sample data */

	POST			= 0x60,
	PUT				= 0x61,			/* No sample data */
};

static void add_uri (proto_tree *, tvbuff_t *, guint, guint);
static void add_headers (proto_tree *, tvbuff_t *);
static void add_header (proto_tree *, tvbuff_t *, tvbuff_t *);
static guint get_value_length (tvbuff_t *, guint, guint *);
static guint add_content_type (proto_tree *, tvbuff_t *, guint, guint *);
static gint get_date_value (tvbuff_t * ,guint ,struct timeval *);
static void add_date_value (tvbuff_t * ,guint ,proto_tree * ,int ,
	tvbuff_t * ,guint ,guint ,struct timeval *, const char *);
static guint add_parameter (proto_tree *, tvbuff_t *, guint);
static guint add_parameter_charset (proto_tree *, tvbuff_t *, guint, guint);
static void add_post_data (proto_tree *, tvbuff_t *, guint);
static void add_post_variable (proto_tree *, tvbuff_t *, guint, guint, guint, guint);
static gint get_long_integer (tvbuff_t *, guint *, guint , guint *);
static void add_capabilities (proto_tree *tree, tvbuff_t *tvb);
static guint get_uintvar (tvbuff_t *, guint, guint);

/* Code to actually dissect the packets */

/* Code to actually dissect the packets */
static void
dissect_redirect(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, dissector_t dissector)
{
	guint8 flags;
	proto_item *ti;
	proto_tree *flags_tree;
	guint8 address_flags_len;
	int address_len;
	proto_tree *atf_tree;
	guint8 bearer_type;
	guint16 port_num = 0;
	guint32 address_ipv4 = 0;
	address redir_address;
	conversation_t *conv;

	flags = tvb_get_guint8 (tvb, offset);
	if (tree) {
		ti = proto_tree_add_uint (tree, hf_wsp_redirect_flags,
		    tvb, offset, 1, flags);
		flags_tree = proto_item_add_subtree (ti, ett_redirect_flags);
		proto_tree_add_boolean (flags_tree, hf_wsp_redirect_permanent,
		    tvb, offset, 1, flags);
		proto_tree_add_boolean (flags_tree, hf_wsp_redirect_reuse_security_session,
		    tvb, offset, 1, flags);
	}
	offset++;
	while (tvb_reported_length_remaining (tvb, offset) > 0) {
		address_flags_len = tvb_get_guint8 (tvb, offset);
		if (tree) {
			ti = proto_tree_add_uint (tree, hf_wsp_redirect_afl,
			    tvb, offset, 1, address_flags_len);
			atf_tree = proto_item_add_subtree (ti, ett_redirect_afl);
			proto_tree_add_boolean (atf_tree, hf_wsp_redirect_afl_bearer_type_included,
			    tvb, offset, 1, address_flags_len);
			proto_tree_add_boolean (atf_tree, hf_wsp_redirect_afl_port_number_included,
			    tvb, offset, 1, address_flags_len);
			proto_tree_add_uint (atf_tree, hf_wsp_redirect_afl_address_len,
			    tvb, offset, 1, address_flags_len);
		}
		offset++;
		if (address_flags_len & BEARER_TYPE_INCLUDED) {
			if (tree) {
				proto_tree_add_item (tree, hf_wsp_redirect_bearer_type,
				    tvb, offset, 1, bo_little_endian);
			}
			offset++;
		}
		if (address_flags_len & PORT_NUMBER_INCLUDED) {
			port_num = tvb_get_ntohs (tvb, offset);
			if (tree) {
				proto_tree_add_uint (tree, hf_wsp_redirect_port_num,
				    tvb, offset, 2, port_num);
			}
			offset += 2;
		} else {
			/*
			 * Redirecting to the same server port number as was
			 * being used, i.e. the source port number of this
			 * redirect.
			 */
			port_num = pinfo->srcport;
		}
		address_len = address_flags_len & ADDRESS_LEN;
		switch (address_len) {

		case 0:		/* no address? */
			break;

		case 4:		/* we assume this is IPv4 */
			tvb_memcpy(tvb, (guint8 *)&address_ipv4, offset, 4);
			if (tree) {
				proto_tree_add_ipv4 (tree, hf_wsp_redirect_ipv4_addr,
				    tvb, offset, 4, address_ipv4);
			}

			redir_address.type = AT_IPv4;
			redir_address.len = 4;
			redir_address.data = (const guint8 *)&address_ipv4;
			conv = find_conversation(&redir_address, &pinfo->dst, 
			    PT_UDP, port_num, 0, NO_PORT_B);
			if (conv == NULL) {
				conv = conversation_new(&redir_address, &pinfo->dst,
				    PT_UDP, port_num, 0, NULL, NO_PORT2);
			}
			conversation_set_dissector(conv, dissector);
			break;

		case 16:	/* XXX - handle this as IPv6? */
		default:
			if (tree) {
				proto_tree_add_item (tree, hf_wsp_redirect_addr,
				    tvb, offset, address_len, bo_little_endian);
			}
			break;
		}
		offset += address_len;
	}
}

static void
dissect_wsp_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
    dissector_t dissector)
{
	frame_data *fdata = pinfo->fd;
	int offset = 0;

	guint8 pdut;
	guint count = 0;
	guint value = 0;
	guint uriLength = 0;
	guint uriStart = 0;
	guint capabilityLength = 0;
	guint capabilityStart = 0;
	guint headersLength = 0;
	guint headerLength = 0;
	guint headerStart = 0;
	guint nextOffset = 0;
	guint contentTypeStart = 0;
	guint contentType = 0;
	tvbuff_t *tmp_tvb;

/* Set up structures we will need to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *wsp_tree = NULL;
/*	proto_tree *wsp_header_fixed; */
	
/* This field shows up as the "Info" column in the display; you should make
   it, if possible, summarize what's in the packet, so that a user looking
   at the list of packets can tell what type of packet it is. */
    
	/* Clear the Info column before we fetch anything from the packet */
	if (check_col(fdata, COL_INFO))
	{
		col_clear(fdata, COL_INFO);
	}

	/* Connection-less mode has a TID first */
	if (	(pinfo->match_port == UDP_PORT_WSP) ||
			(pinfo->match_port == UDP_PORT_WTLS_WSP))
	{
		offset++;
	};

	/* Find the PDU type */
	pdut = tvb_get_guint8 (tvb, offset);

	/* Develop the string to put in the Info column */
	if (check_col(fdata, COL_INFO))
	{
		col_add_fstr(fdata, COL_INFO, "WSP %s",
			val_to_str (pdut, vals_pdu_type, "Unknown PDU type (0x%02x)"));
	};

/* In the interest of speed, if "tree" is NULL, don't do any work not
   necessary to generate protocol tree items. */
	if (tree) {
		ti = proto_tree_add_item(tree, proto_wsp, tvb, 0,
		    tvb_length(tvb), bo_little_endian);
	        wsp_tree = proto_item_add_subtree(ti, ett_wsp);

/* Code to process the packet goes here */
/*
		wsp_header_fixed = proto_item_add_subtree(ti, ett_header );
*/

		/* Add common items: only TID and PDU Type */

		/* If this is connectionless, then the TID Field is always first */
		if (	(pinfo->match_port == UDP_PORT_WSP) ||
				(pinfo->match_port == UDP_PORT_WTLS_WSP))
		{
			ti = proto_tree_add_item (wsp_tree, hf_wsp_header_tid,tvb,
				0,1,bo_little_endian);
		}

		ti = proto_tree_add_item(
				wsp_tree, 		/* tree */
				hf_wsp_header_pdu_type, /* id */
				tvb, 
				offset, 		/* start of high light */
				1,			/* length of high light */
				bo_little_endian	/* value */
		     );
	}
	offset++;

	switch (pdut)
	{
		case CONNECT:
			if (tree) {
				ti = proto_tree_add_item (wsp_tree, hf_wsp_version_major,tvb,offset,1,bo_little_endian);
				ti = proto_tree_add_item (wsp_tree, hf_wsp_version_minor,tvb,offset,1,bo_little_endian);
				offset++;
				capabilityStart = offset;
				count = 0;	/* Initialise count */
				capabilityLength = tvb_get_guintvar (tvb, offset, &count);
				offset += count;
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_capability_length,tvb,capabilityStart,count,capabilityLength);

				headerStart = offset;
				count = 0;	/* Initialise count */
				headerLength = tvb_get_guintvar (tvb, offset, &count);
				offset += count;
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_header_length,tvb,headerStart,count,headerLength);
				if (capabilityLength > 0)
				{
					tmp_tvb = tvb_new_subset (tvb, offset, capabilityLength, capabilityLength);
					add_capabilities (wsp_tree, tmp_tvb);
					offset += capabilityLength;
				}

				if (headerLength > 0)
				{
					tmp_tvb = tvb_new_subset (tvb, offset, headerLength, headerLength);
					add_headers (wsp_tree, tmp_tvb);
				}
			}

			break;

		case CONNECTREPLY:
			if (tree) {
				count = 0;	/* Initialise count */
				value = tvb_get_guintvar (tvb, offset, &count);
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_server_session_id,tvb,offset,count,value);
				offset += count;

				capabilityStart = offset;
				count = 0;	/* Initialise count */
				capabilityLength = tvb_get_guintvar (tvb, offset, &count);
				offset += count;
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_capability_length,tvb,capabilityStart,count,capabilityLength);

				headerStart = offset;
				count = 0;	/* Initialise count */
				headerLength = tvb_get_guintvar (tvb, offset, &count);
				offset += count;
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_header_length,tvb,headerStart,count,headerLength);
				if (capabilityLength > 0)
				{
					tmp_tvb = tvb_new_subset (tvb, offset, capabilityLength, capabilityLength);
					add_capabilities (wsp_tree, tmp_tvb);
					offset += capabilityLength;
				}

				if (headerLength > 0)
				{

					/*
					ti = proto_tree_add_item (wsp_tree, hf_wsp_headers_section,tvb,offset,headerLength,bo_little_endian);
					wsp_headers = proto_item_add_subtree( ti, ett_headers );
					*/
					tmp_tvb = tvb_new_subset (tvb, offset, headerLength, headerLength);
					add_headers (wsp_tree, tmp_tvb);
				}
			}

			break;

		case REDIRECT:
			dissect_redirect(tvb, offset, pinfo, wsp_tree,
			  dissector);
			break;

		case DISCONNECT:
			if (tree) {
				count = 0;	/* Initialise count */
				value = tvb_get_guintvar (tvb, offset, &count);
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_server_session_id,tvb,offset,count,value);
			}
			break;

		case GET:
			if (tree) {
				count = 0;	/* Initialise count */
				/* Length of URI and size of URILen field */
				value = tvb_get_guintvar (tvb, offset, &count);
				nextOffset = offset + count;
				add_uri (wsp_tree, tvb, offset, nextOffset);
				offset += (value+count); /* VERIFY */
				tmp_tvb = tvb_new_subset (tvb, offset, -1, -1);
				add_headers (wsp_tree, tmp_tvb);
			}
			break;

		case POST:
			if (tree) {
				uriStart = offset;
				count = 0;	/* Initialise count */
				uriLength = tvb_get_guintvar (tvb, offset, &count);
				headerStart = uriStart+count;
				count = 0;	/* Initialise count */
				headersLength = tvb_get_guintvar (tvb, headerStart, &count);
				offset = headerStart + count;

				add_uri (wsp_tree, tvb, uriStart, offset);
				offset += uriLength;

				ti = proto_tree_add_item (wsp_tree, hf_wsp_header_length,tvb,headerStart,count,bo_little_endian);

				contentTypeStart = offset;
				nextOffset = add_content_type (wsp_tree, tvb, offset, &contentType);

				/* Add headers subtree that will hold the headers fields */
				/* Runs from nextOffset for value-(length of content-type field)*/
				headerLength = headersLength-(nextOffset-contentTypeStart);
				tmp_tvb = tvb_new_subset (tvb, nextOffset, headerLength, headerLength);
				add_headers (wsp_tree, tmp_tvb);

				/* TODO: Post DATA */
				/* Runs from start of headers+headerLength to end of frame */
				offset = nextOffset+headerLength;
				tmp_tvb = tvb_new_subset (tvb, offset, tvb_reported_length (tvb)-offset, tvb_reported_length (tvb)-offset);
				add_post_data (wsp_tree, tmp_tvb, contentType);
			}
			break;

		case REPLY:
			if (tree) {
				ti = proto_tree_add_item (wsp_tree, hf_wsp_header_status,tvb,offset,1,bo_little_endian);
				count = 0;	/* Initialise count */
				value = tvb_get_guintvar (tvb, offset+1, &count);
				nextOffset = offset + 1 + count;
				ti = proto_tree_add_uint (wsp_tree, hf_wsp_header_length,tvb,offset+1,count,value);

				contentTypeStart = nextOffset;
				nextOffset = add_content_type (wsp_tree, tvb, nextOffset, &contentType);

				/* Add headers subtree that will hold the headers fields */
				/* Runs from nextOffset for value-(length of content-type field)*/
				headerLength = value-(nextOffset-contentTypeStart);
				tmp_tvb = tvb_new_subset (tvb, nextOffset, headerLength, headerLength);
				add_headers (wsp_tree, tmp_tvb);
				offset += count+value+1;

				/* TODO: Data - decode WMLC */
				/* Runs from offset+1+count+value+1 to end of frame */
				if (offset < tvb_reported_length (tvb))
				{
					ti = proto_tree_add_item (wsp_tree, hf_wsp_reply_data,tvb,offset,tvb_length_remaining(tvb, offset),bo_little_endian);
				}
			}
			break;
	}
}

/*
 * Called directly from UDP.
 * Put "WSP" into the "Protocol" column.
 */
static void
dissect_wsp_fromudp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if (check_col(pinfo->fd, COL_PROTOCOL))
		col_set_str(pinfo->fd, COL_PROTOCOL, "WSP" );

	dissect_wsp_common(tvb, pinfo, tree, dissect_wsp_fromudp);
}

/*
 * Called from a higher-level WAP dissector.
 * Leave the "Protocol" column alone - the dissector calling us should
 * have set it.
 */
static void
dissect_wsp_fromwap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	/*
	 * XXX - what about WTLS->WTP->WSP?
	 */
	dissect_wsp_common(tvb, pinfo, tree, dissect_wtp_fromudp);
}

static void
add_uri (proto_tree *tree, tvbuff_t *tvb, guint URILenOffset, guint URIOffset)
{
	proto_item *ti;
	guint8 terminator = 0;
	char *newBuffer;

	guint count = 0;
	guint uriLen = tvb_get_guintvar (tvb, URILenOffset, &count);

	ti = proto_tree_add_uint (tree, hf_wsp_header_uri_len,tvb,URILenOffset,count,uriLen);

	/* If string doesn't end with a 0x00, we need to add one to be on the safe side */
	terminator = tvb_get_guint8 (tvb, URIOffset+uriLen-1);
	if (terminator != 0)
	{
		newBuffer = g_malloc (uriLen+1);
		strncpy (newBuffer, tvb_get_ptr (tvb, URIOffset, uriLen), uriLen);
		newBuffer[uriLen] = 0;
		ti = proto_tree_add_string (tree, hf_wsp_header_uri,tvb,URIOffset,uriLen,newBuffer);
		g_free (newBuffer);
	}
	else
	{
		ti = proto_tree_add_item (tree, hf_wsp_header_uri,tvb,URIOffset,uriLen,bo_little_endian);
	}
}

static void
add_headers (proto_tree *tree, tvbuff_t *tvb)
{
	proto_item *ti;
	proto_tree *wsp_headers;
	guint offset = 0;
	guint headersLen = tvb_reported_length (tvb);
	guint8 headerStart = 0;
	guint peek = 0;
	tvbuff_t *header_buff;
	tvbuff_t *value_buff;
	guint count = 0;
	guint valueStart = 0;
	guint valueEnd = 0;
	guint pageCode = 1;

#ifdef DEBUG
	fprintf (stderr, "dissect_wsp: Offset is %d, size is %d\n", offset, headersLen);
#endif

	/* End of buffer */
	if (headersLen <= 0)
	{
		return;
	}

#ifdef DEBUG
	fprintf (stderr, "dissect_wsp: Headers to process\n");
#endif

	ti = proto_tree_add_item (tree, hf_wsp_headers_section,tvb,offset,headersLen,bo_little_endian);
	wsp_headers = proto_item_add_subtree( ti, ett_headers );

	/* Parse Headers */

	while (offset < headersLen)
	{
		/* Loop round each header */
		headerStart = offset;
		peek = tvb_get_guint8 (tvb, headerStart);

		if (peek < 32)		/* Short-cut shift delimeter */
		{
			pageCode = peek;
			proto_tree_add_uint (wsp_headers, hf_wsp_header_shift_code, tvb , offset, 1, peek);
			offset+=1;
			continue;
		}
		else if (peek == 0x7F)	/* Shift delimeter */
		{
			pageCode = tvb_get_guint8(tvb,offset+1);
			proto_tree_add_uint (wsp_headers, hf_wsp_header_shift_code, tvb , offset, 2, pageCode);
			offset+=2;
			continue;
		}
		else if (peek < 127)
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: header: application-header start %d (0x%02X)\n", peek, peek);
#endif
			while (tvb_get_guint8 (tvb, offset++)) { /* Do nothing, just look for NULL */ }
		}
		else if (peek & 0x80)	/* Well-known header */
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: header: well-known %d (0x%02X)\n", peek, peek);
#endif
			offset++;
		}

		/* Get value part of header */
		valueStart = offset;
		peek = tvb_get_guint8 (tvb, valueStart);
		if (peek <= 30)
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: Looking for %d octets\n", peek);
#endif
			/* VERIFY: valueStart++; */
			valueEnd = offset+1+peek;
			offset += (peek+1);
		}
		else if (peek == 31)
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: Looking for uintvar octets\n");
#endif
			count = 0;	/* Initialise count */
			tvb_get_guintvar (tvb, valueStart, &count);
			valueEnd = offset+1+count;
			offset += (count+1);
		}
		else if (peek <= 127)
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: Looking for NULL-terminated string\n");
#endif
			valueEnd = valueStart+1;
			while (tvb_get_guint8 (tvb, valueEnd++)) { /* Do nothing, just look for NULL */ }
			offset = valueEnd;
		}
		else
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: Value is %d\n", (peek & 0x7F));
#endif
			valueEnd = offset+1;
			offset++;
		}
#ifdef DEBUG
		fprintf (stderr, "dissect_wsp: Creating value buffer from offset %d, size=%d\n", headerStart, (offset-headerStart));
#endif

		if (pageCode == 1)
		{
			header_buff = tvb_new_subset (tvb, headerStart, (offset-headerStart), (offset-headerStart));
			value_buff = tvb_new_subset (tvb, valueStart, (valueEnd-valueStart), (valueEnd-valueStart));
			add_header (wsp_headers, header_buff, value_buff);
		}
		else 
		{
			proto_tree_add_text (wsp_headers, tvb , headerStart, valueEnd-headerStart,
				       "Unsupported Header (0x%02X)", (tvb_get_guint8 (tvb, headerStart) & 0x7F));
		}

	}
}

static void
add_header (proto_tree *tree, tvbuff_t *header_buff, tvbuff_t *value_buff)
{
	guint offset = 0;
	guint valueStart = 0;
	guint8 headerType = 0;
	proto_item *ti;
	guint headerLen = tvb_reported_length (header_buff);
	guint valueLen = tvb_reported_length (value_buff);
	guint peek = 0;
	struct timeval timeValue;
	guint offsetNew = 0;
	guint value = 0;
	guint valueLength = 0;
	char valString[100];
	char valNum1[100];
	char *valMatch = NULL;
	double q_value = 1.0;

	/* Initialise time values */
	timeValue.tv_sec=0;
	timeValue.tv_usec = 0;

	headerType = tvb_get_guint8 (header_buff, 0);
	peek = tvb_get_guint8 (value_buff, 0);
#ifdef DEBUG
	fprintf (stderr, "dissect_wsp: Got header 0x%02x\n", headerType);
	fprintf (stderr, "dissect_wsp: First value octet is 0x%02x\n", peek);
#endif

	if (headerType == 0x7F)
	{
	}
	else if (headerType < 0x1F)
	{
	}
	else if (headerType & 0x80)
	{
		headerType = headerType & 0x7F;
		switch (headerType)
		{
			case 0x00:		/* Accept */
				if (peek & 0x80) /* Constrained-media (short-integer) */
				{
					proto_tree_add_uint (tree, hf_wsp_header_accept, header_buff, offset, headerLen, (peek & 0x7F));
					break;
				}
				if (peek >= 32) /* Constrained-media (text) */
				{
					proto_tree_add_string (tree, hf_wsp_header_accept_str,header_buff,offset,headerLen,
								tvb_get_ptr (value_buff, 0, valueLen));
					break;
				}
				/* general form (short length) */
				offsetNew = offset+1;
				valueLength = tvb_get_guint8 (value_buff, offsetNew);
				if (valueLength & 0x80) /* short integer */
				{
					valMatch = match_strval(valueLength & 0x7f, vals_content_types);
					offsetNew++;
				}
				else if (valueLength < 32) /* long integer */
				{
					if (get_long_integer(value_buff,&offsetNew,offset+peek,&value) < 0)
					{
						fprintf (stderr, "dissect_wsp: accept get_long_interger invalid\n");
					}
					valMatch = match_strval(value, vals_content_types);
					offsetNew+=valueLength;
					if (valMatch == NULL)
					{
						snprintf(valNum1,100,"Unsupported type (%d)",value);
						valMatch = valNum1;
					}
				}
				else /* text */
				{
					/* TODO */
				}
				if ((offsetNew - offset) < peek) /* Parameter */
				{
					value = tvb_get_guint8 (value_buff, offsetNew);
					offsetNew++;
					if (value & 0x80) /* well Know type */
					{
						switch (value & 0x7f)
						{
							case 0x03 :
								valueLength = tvb_get_guint8 (value_buff, offsetNew);
								if (valueLength & 0x80) /* short integer */
								{
									value = valueLength & 0x7F;
									offsetNew++;
								}
								else if (valueLength < 32) /* Long integer */
								{
									if (get_long_integer(value_buff,&offsetNew,offset+peek,&value) < 0)
									{
										fprintf (stderr, "dissect_wsp: accept get_long_interger parameter invalid\n");
									}
								}
								snprintf (valString, 100, "%s type %d", valMatch,value);
								break;
							case 0x00 :
							case 0x01 :
							case 0x02 :
							case 0x04 :
							default :
								break;
						}
					}
				}
				else  /* no parameter */
				{
					snprintf (valString, 100, "%s", valMatch);
				}
				/* Add string to tree */
				proto_tree_add_string (tree, hf_wsp_header_accept_str,
						       header_buff, 0, headerLen, valString);
					
				break;

			case 0x01:		/* Accept-Charset */
				if (peek <= 31)				/* Accept-charset-general-form */
				{
					/* Get Value-Length */
					valueLength = get_value_length (value_buff, offset,
						&valueStart);
					offset = valueStart;

					peek = tvb_get_guint8 (value_buff, offset);
					if ((peek >= 0x80) || (peek <= 30))	/* Well-known-charset */
					{
						if (peek == 0x80)		/* Any */
						{
							value = peek;
							offset++;
						}
						else if (peek & 0x80)	/* Short-Integer */
						{
							value = peek & 0x7F;
							offset++;
						}
						else if (peek <= 30)	/* Long-Integer */
						{
							offset++;
	
							switch (peek)
							{
								case 1:
									value = tvb_get_guint8 (value_buff, offset);
									break;
								case 2:
									value = tvb_get_ntohs (value_buff, offset);
									break;
								case 3:
									value = tvb_get_ntoh24 (value_buff, offset);
									break;
								case 4:
									value = tvb_get_ntohl (value_buff, offset);
									break;
								default:
									/* TODO: Need to read peek octets */
									value = 0;
									fprintf (stderr, "dissect_wsp: accept-charset size %d NYI\n", peek);
									break;
							}
							offset += peek;
						}
						valMatch = match_strval(value, vals_character_sets);
					}
					else						/* Assume Token-text */
					{
						fprintf (stderr, "dissect_wsp: Accept-Charset Token-text NYI\n");
					}

					/* Any remaining data relates to Q-Value */
					if (offset < valueLen)
					{
						peek = tvb_get_guintvar (value_buff, offset, NULL);
						if (peek <= 100) {
							peek = (peek - 1) * 10;
						}
						else {
							peek -= 100;
						}
						q_value = peek/1000.0;
					}

					/* Build string including Q values if present */
					if (q_value == 1.0)			/* Default */
					{
						if (valMatch == NULL)
						{
							snprintf (valString, 100, "Unknown (%X)", peek);
						}
						else
						{
							snprintf (valString, 100, "%s", valMatch);
						}
					}
					else
					{
						if (valMatch == NULL)
						{
							snprintf (valString, 100, "Unknown (%X); Q=%5.3f", peek,q_value);
						}
						else
						{
							snprintf (valString, 100, "%s; Q=%5.3f", valMatch,q_value);
						}
					}
                                        /* Add string to tree */
                                        proto_tree_add_string (tree, hf_wsp_header_accept_charset_str,
				                               header_buff, 0, headerLen, valString);

				}
				else						/* Constrained-charset */
				{
					if (peek == 0x80)		/* Any-charset */
					{
						proto_tree_add_string (tree, hf_wsp_header_accept_charset,
							header_buff, offset, headerLen,
							"*");
					}
					else if (peek & 0x80)	/* Short-Integer */
					{
						proto_tree_add_uint (tree, hf_wsp_header_accept_charset,
							header_buff, offset, headerLen, (peek & 0x7F) );
					}
					else					/* Assume *TEXT */
					{
						proto_tree_add_string (tree, hf_wsp_header_accept_charset,
							header_buff, offset, headerLen,
							tvb_get_ptr (value_buff, 0, valueLen));
					}
				}
				break;

			case 0x03:		/* Accept-Language */
				if (peek < 31)
				{
					/* Peek contains the number of octets to follow */
					switch (peek)
					{
						case 1:
							proto_tree_add_uint (tree, hf_wsp_header_accept_language, header_buff, offset, 
								headerLen, tvb_get_guint8 (value_buff, 1) );
						break;
						case 2:
							proto_tree_add_uint (tree, hf_wsp_header_accept_language, header_buff, offset, 
								headerLen, tvb_get_ntohs (value_buff, 1) );
						break;
						case 4:
							proto_tree_add_uint (tree, hf_wsp_header_accept_language, header_buff, offset, 
								headerLen, tvb_get_ntohl (value_buff, 1) );
						break;
						default:
							fprintf (stderr, "dissect_wsp: accept-language size %d NYI\n", peek);
					}
				}
				else if (peek & 0x80)
				{
					proto_tree_add_uint (tree, hf_wsp_header_accept_language, header_buff, offset, headerLen, (peek & 0x7F) );
				}
				else
				{
					proto_tree_add_string (tree, hf_wsp_header_accept_language_str, header_buff, offset,headerLen,
						tvb_get_ptr (value_buff, 0, valueLen));
				}
				break;

			case 0x04:		/* Accept-Ranges */
				if ((peek == 128) || (peek == 129))
				{
					proto_tree_add_uint (tree, hf_wsp_header_accept_ranges, header_buff, offset, headerLen, peek);
				}
				else
				{
					fprintf (stderr, "dissect_wsp: accept-ranges NYI\n");
				}
				
				break;

			case 0x05:		/* Age */
				switch (valueLen)
				{
					case 1:
						proto_tree_add_uint (tree, hf_wsp_header_age, header_buff, offset, headerLen, tvb_get_guint8 (value_buff, 0));
						break;
					case 2:
						proto_tree_add_uint (tree, hf_wsp_header_age, header_buff, offset, headerLen, tvb_get_ntohs (value_buff, 0));
						break;
					case 3:
						proto_tree_add_uint (tree, hf_wsp_header_age, header_buff, offset, headerLen, tvb_get_ntoh24 (value_buff, 0));
						break;
					case 4:
						proto_tree_add_uint (tree, hf_wsp_header_age, header_buff, offset, headerLen, tvb_get_ntohl (value_buff, 0));
						break;
				};
				break;

			case 0x08:		/* Cache-Control */
				if (peek & 0x80)
				{
					if (valueLen == 1)	/* Well-known value */
					{
						proto_tree_add_uint (tree, hf_wsp_header_cache_control, header_buff, offset, headerLen, peek);
					}
					else
					{
						if ((peek == 0x82) || (peek == 0x83) || (peek == 0x84))	/* Delta seconds value to follow */
						{
							value = tvb_get_guint8 (value_buff, 1);
							if (value & 0x80)
							{
								proto_tree_add_text (tree, header_buff, 0,
									headerLen, "Cache-Control: %s %d (0x%02X)",
							    	val_to_str (peek, vals_cache_control,
							        	"Unknown (0x%02x)"),
							        (value & 0x7F), peek);
							}
							else
							{
								fprintf (stderr, "dissect_wsp: Cache-Control integer value Delta seconds NYI\n");
							}
						}
						else if ((peek == 0x80) || (peek == 0x87))	/* Fields to follow */
						{
							fprintf (stderr, "dissect_wsp: Cache-Control field values NYI\n");
						}
						else
						{
							fprintf (stderr, "dissect_wsp: Cache-Control cache extension NYI\n");
						}
					}
				}
				else
				{
					fprintf (stderr, "dissect_wsp: Cache-Control cache extension NYI\n");
				}
				break;
				
			case 0x0D:		/* Content-Length */
				if (peek < 31)
				{
					switch (peek)
					{
						case 1:
							proto_tree_add_uint (tree,
								hf_wsp_header_content_length, header_buff, offset,
								headerLen, tvb_get_guint8 (value_buff, 1) );
							break;
						case 2:
							proto_tree_add_uint (tree,
								hf_wsp_header_content_length, header_buff, offset,
								headerLen, tvb_get_ntohs (value_buff, 1) );
							break;
						case 3:
							proto_tree_add_uint (tree,
								hf_wsp_header_content_length, header_buff, offset,
								headerLen, (tvb_get_ntohs (value_buff, 1) << 8) +
								tvb_get_guint8 (value_buff, 3) );
							break;
						case 4:
							proto_tree_add_uint (tree,
								hf_wsp_header_content_length, header_buff, offset,
								headerLen, tvb_get_ntohl (value_buff, 1) );
							break;
						default:
							fprintf (stderr, "dissect_wsp: accept-charset size %d NYI\n", peek);
					}
				}
				else if (peek & 0x80)
				{
					proto_tree_add_uint (tree, hf_wsp_header_content_length, header_buff, offset, headerLen, (peek & 0x7F));
				}
				else
				{
					fprintf (stderr, "dissect_wsp: Content-Length long-integer size NYI\n");
				}
				break;
				
			case 0x12:		/* Date */
				add_date_value (value_buff, 0, tree,
					hf_wsp_header_date, header_buff, offset,
					headerLen, &timeValue, "Date");
				break;

			case 0x13:		/* Etag */
				ti = proto_tree_add_string (tree, hf_wsp_header_etag,header_buff,offset,headerLen,tvb_get_ptr (value_buff, 0, valueLen));
				break;

			case 0x14:		/* Expires */
				add_date_value (value_buff, 0, tree,
					hf_wsp_header_expires, header_buff, offset,
					headerLen, &timeValue, "Expires");
				break;

			case 0x17:		/* If-Modified-Since */
				add_date_value (value_buff, 0, tree,
					hf_wsp_header_if_modified_since, header_buff, offset,
					headerLen, &timeValue, "If-Modified-Since");
				break;
				
			case 0x1C:		/* Location */
				ti = proto_tree_add_string (tree, hf_wsp_header_location,header_buff,offset,headerLen,tvb_get_ptr (value_buff, 0, valueLen));
				break;

			case 0x1D:		/* Last-Modified */
				add_date_value (value_buff, 0, tree,
					hf_wsp_header_last_modified, header_buff, offset,
					headerLen, &timeValue, "Last-Modified");
				break;
				
			case 0x1F:		/* Pragma */
				if (peek == 0x80)
				{
					proto_tree_add_text (tree, header_buff, 0, headerLen, "Pragma: No-cache");
				}
				else
				{
					proto_tree_add_text (tree, header_buff, 0, headerLen, "Unsupported Header (0x%02X)", (tvb_get_guint8 (header_buff, 0) & 0x7F));
				}
				break;
				
			case 0x26:		/* Server */
				ti = proto_tree_add_string (tree, hf_wsp_header_server,header_buff,offset,headerLen,tvb_get_ptr (value_buff, 0, valueLen));
				break;

			case 0x27:		/* Transfer encoding */
				if (peek & 0x80)
				{
					proto_tree_add_uint (tree, hf_wsp_header_transfer_encoding,
						header_buff, offset, headerLen, peek);
				}
				else
				{
					proto_tree_add_string (tree,
						hf_wsp_header_transfer_encoding_str, header_buff, offset,
						headerLen,tvb_get_ptr (value_buff, 0, valueLen));
				}
				break;

			case 0x29:		/* User-Agent */
				ti = proto_tree_add_string (tree, hf_wsp_header_user_agent,header_buff,offset,headerLen,tvb_get_ptr (value_buff, 0, valueLen));
				break;

			case 0x2B:		/* Via */
				ti = proto_tree_add_string (tree, hf_wsp_header_via, header_buff,
					offset, headerLen, tvb_get_ptr (value_buff, 0, valueLen));
				break;

			default:
				ti = proto_tree_add_text (tree, header_buff, 0, headerLen, "Unsupported Header (0x%02X)", (tvb_get_guint8 (header_buff, 0) & 0x7F));
				break;
		}
	}
	else
	{
		/* Special case header X-WAP.TOD that is sometimes followed
		 * by a 4-byte date value */
		if (strncasecmp ("x-wap.tod", tvb_get_ptr (header_buff, 0, headerLen), 9) == 0)
		{
			peek = tvb_get_guint8 (value_buff,offset);
			if (peek < 31) 
			{
				timeValue.tv_usec = 0;
				switch (peek)
				{
					case 1:
						timeValue.tv_sec = tvb_get_guint8 (value_buff, 1);
						break;
					case 2:
						timeValue.tv_sec = tvb_get_ntohs (value_buff, 1);
						break;
					case 3:
						timeValue.tv_sec = tvb_get_ntoh24 (value_buff, 1);
						break;
					case 4:
						timeValue.tv_sec = tvb_get_ntohl (value_buff, 1);
						break;
					default:
						timeValue.tv_sec = 0;
						fprintf (stderr, "dissect_wsp: X-WAP.TOD NYI\n");
						break;
				}
				ti = proto_tree_add_time (tree, hf_wsp_header_x_wap_tod, header_buff, offset, headerLen, &timeValue);
			}
			else
		       	{
				fprintf (stderr, "dissect_wsp: X-WAP.TOD peek %X NYI\n",peek);
			}
		}
		else
		{
			ti = proto_tree_add_text (tree, header_buff, 0, headerLen, "%s: %s", tvb_get_ptr (header_buff, 0, headerLen), tvb_get_ptr (value_buff, 0, valueLen));
		}
	}

}

static void
add_capabilities (proto_tree *tree, tvbuff_t *tvb)
{
	proto_item *ti;
	proto_tree *wsp_capabilities;
	guint offset = 0;
	guint offsetStr = 0;
	guint capabilitiesLen = tvb_reported_length (tvb);
	guint8 capabilitiesStart = 0;
	guint peek = 0;
	guint length = 0;
	guint value = 0;
	guint i;
	char valString[200];

#ifdef DEBUG
	fprintf (stderr, "dissect_wsp: Offset is %d, size is %d\n", offset, capabilitiesLen);
#endif

	/* End of buffer */
	if (capabilitiesLen <= 0)
	{
		fprintf (stderr, "dissect_wsp: Capabilities = 0\n");
		return;
	}

#ifdef DEBUG
	fprintf (stderr, "dissect_wsp: capabilities to process\n");
#endif

	ti = proto_tree_add_item (tree, hf_wsp_capabilities_section,tvb,offset,capabilitiesLen,bo_little_endian);
	wsp_capabilities = proto_item_add_subtree( ti, ett_capabilities );

	/* Parse Headers */

	while (offset < capabilitiesLen)
	{
		/* Loop round each header */
		capabilitiesStart = offset;
		length = tvb_get_guint8 (tvb, capabilitiesStart);

		if (length >= 127)		/* length */
		{
#ifdef DEBUG
			fprintf (stderr, "dissect_wsp: capabilities length invalid %d\n",length);
#endif
			offset+=length;
			continue;
		}
		offset++;
		peek = tvb_get_guint8 (tvb, offset);
		offset++;
		switch (peek & 0x7f)
		{
			case 0x00 : /* Client-SDU-Size */
				value = get_uintvar (tvb, offset, length+capabilitiesStart+1);
				proto_tree_add_uint (wsp_capabilities, hf_wsp_capabilities_client_SDU, tvb, capabilitiesStart, length+1, value);
				break;
			case 0x01 : /* Server-SDU-Size */
				value = get_uintvar (tvb, offset, length+capabilitiesStart+1);
				proto_tree_add_uint (wsp_capabilities, hf_wsp_capabilities_server_SDU, tvb, capabilitiesStart, length+1, value);
				break;
			case 0x02 : /* Protocol Options */ 
				value = get_uintvar (tvb, offset, length+capabilitiesStart+1);
				i = 0;
				valString[0]=0;
				if (value & 0x80)
				{
					i += snprintf(valString+i,200-1,"%s","(Confirmed push facility) ");
				}
				if (value & 0x40)
				{
					i += snprintf(valString+i,200-1,"%s","(Push facility) ");
				}
				if (value & 0x20)
				{
					i += snprintf(valString+i,200-1,"%s","(Session resume facility) ");
				}
				if (value & 0x10)
				{
					i += snprintf(valString+i,200-1,"%s","(Acknowledgement headers) ");
				}
				proto_tree_add_string(wsp_capabilities, hf_wsp_capabilities_protocol_opt, tvb, capabilitiesStart, length+1, valString);
				break;
			case 0x03 : /* Method-MOR */ 
				value = tvb_get_guint8(tvb, offset);
				proto_tree_add_uint (wsp_capabilities, hf_wsp_capabilities_method_MOR, tvb, capabilitiesStart, length+1, value);
				break;
			case 0x04 : /* Push-MOR */ 
				value = tvb_get_guint8(tvb, offset);
				proto_tree_add_uint (wsp_capabilities, hf_wsp_capabilities_push_MOR, tvb, capabilitiesStart, length+1, value);
				break;
				break;
			case 0x05 : /* Extended Methods */ 
				offsetStr = offset;
				offset++;
				i = 0;
				while ((offsetStr-capabilitiesStart) <= length)
				{
					value = tvb_get_guint8(tvb, offsetStr);
					i += snprintf(valString+i,200-i,"(%d - ",value);
					offsetStr++;
					for (;(valString[i] = tvb_get_guint8(tvb, offsetStr));i++,offsetStr++);
					offsetStr++;
					valString[i++] = ')';
					valString[i++] = ' ';
				}
				valString[i]=0;
				proto_tree_add_string(wsp_capabilities, hf_wsp_capabilities_extended_methods, tvb, capabilitiesStart, length+1, valString);
				break;
			case 0x06 : /* Header Code Pages */ 
				offsetStr = offset;
				offset++;
				i = 0;
				while ((offsetStr-capabilitiesStart) <= length)
				{
					value = tvb_get_guint8(tvb, offsetStr);
					i += snprintf(valString+i,200-i,"(%d - ",value);
					offsetStr++;
					for (;(valString[i] = tvb_get_guint8(tvb, offsetStr));i++,offsetStr++);
					offsetStr++;
					valString[i++] = ')';
					valString[i++] = ' ';
				}
				valString[i]=0;
				proto_tree_add_string(wsp_capabilities, hf_wsp_capabilities_header_code_pages, tvb, capabilitiesStart, length+1, valString);
				break;
			case 0x07 : /* Aliases */
				break;
			default:
				proto_tree_add_text (wsp_capabilities, tvb , capabilitiesStart, length+1,
				       "Unsupported Header (0x%02X)", peek & 0x7F);
				break;
		}
		offset=capabilitiesStart+length+1;
	}
}

static guint
get_uintvar (tvbuff_t *tvb, guint offset, guint offsetEnd)
{
	guint value = 0;
	guint octet;

	do
	{
       		octet = tvb_get_guint8 (tvb, offset);
		offset++;
		value <<= 7;
		value += octet & 0x7f;
	}
	while ((offsetEnd > offset) && (octet & 0x80));
	return value;
}

static guint
get_value_length (tvbuff_t *tvb, guint offset, guint *nextOffset)
{
	guint value = 0;
	guint count = 0;
	guint octet = tvb_get_guint8 (tvb, offset);

	if (octet <= 30)	/* Short length */
	{
		value = octet;
		*nextOffset = offset+1;
	}
	else if (octet == 31)
	{
		value = tvb_get_guintvar (tvb, offset+1, &count);
		*nextOffset = offset+1+count;
	}
	else
	{
		fprintf (stderr, "dissect_wsp: get_value_length: case NYI\n");
	}

	return (value);
}

static guint
add_content_type (proto_tree *tree, tvbuff_t *tvb, guint offset, guint *contentType)
{
	proto_tree *contentTypeTree;
	guint nextOffset = offset;
	guint fieldLength = 0;
	guint octet = tvb_get_guint8 (tvb, offset);
	guint totalSizeOfField = 0;

	if (octet <= 31)
	{
		fieldLength = get_value_length (tvb, offset, &nextOffset);
		totalSizeOfField = (nextOffset-offset)+fieldLength;
	}
	else if (octet & 0x80)
	{
		fieldLength = 1;
		totalSizeOfField = 1;
	}
	else
	{
		fprintf (stderr, "dissect-wsp: Content-type is un-supported\n");
	}

	*contentType = (tvb_get_guint8 (tvb, nextOffset) & 0x7F);
	contentTypeTree = proto_tree_add_uint (tree, hf_wsp_content_type, tvb, offset, totalSizeOfField, (tvb_get_guint8(tvb,nextOffset++) & 0x7F));

	while (nextOffset < (offset+totalSizeOfField))
	{
		/* add_parameter */
		nextOffset = add_parameter (contentTypeTree, tvb, nextOffset);
	}

	return (offset+totalSizeOfField);
}

/* Utility function to extract date values from the packet */
static gint
get_date_value (tvbuff_t *buffer, guint offset, struct timeval *timeValue)
{
	guint ShortLength = 0;

	/* Initialise time values */
	timeValue->tv_sec=0;
	timeValue->tv_usec = 0;

	/* Date values are encoded as: Short-length Multi-octet-integer
	 * Where Short-length = 0-30
	 */
	ShortLength = tvb_get_guint8 (buffer, offset++);
	if (ShortLength > 30)
	{
		fprintf (stderr, "dissect_wsp: Invalid Date-value (Short-length=%d)\n",
			ShortLength);
		return (-1);
	}

	switch (ShortLength)
	{
		case 1:
			timeValue->tv_sec = tvb_get_guint8 (buffer, offset);
			break;
		case 2:
			timeValue->tv_sec = tvb_get_ntohs (buffer, offset);
			break;
		case 3:
			timeValue->tv_sec = tvb_get_ntoh24 (buffer, offset);
			break;
		case 4:
			timeValue->tv_sec = tvb_get_ntohl (buffer, offset);
			break;
		default:
			fprintf (stderr, "dissect_wsp: Date-value Short-length of %d NYI\n",
				ShortLength);
			return (-1);
			break;
	}

	return (0);
}

/* Utility function to add a date value to the protocol tree */
static void
add_date_value (tvbuff_t *buffer, guint offset, proto_tree *tree,
		int header, tvbuff_t *headerBuffer, guint headerOffset,
		guint headerLen, struct timeval *timeValue, const char *fieldName)
{
	/* Attempt to get the date value from the buffer */
	if (get_date_value (buffer, offset, timeValue) == 0)
	{
		/* If successful, add it to the protocol tree */
		proto_tree_add_time (tree, header, headerBuffer, headerOffset,
			headerLen, timeValue);
	}
	else
	{
		fprintf (stderr, "dissect_wsp: Invalid %s value\n", fieldName);
	}
}

static guint
add_parameter (proto_tree *tree, tvbuff_t *tvb, guint offset)
{
	guint octet = tvb_get_guint8 (tvb, offset);
	if (octet & 0x80)	/* Short integer */
	{
		offset++;
		octet = octet & 0x7F;
		switch ( octet )
		{
			case 0x01:
				offset = add_parameter_charset (tree, tvb, offset, offset-1);
				break;

			default:
				fprintf (stderr, "dissect-wsp: add_parameter octet=0x%02x\n", octet);
		};
	}
	else
	{
		fprintf (stderr, "dissect-wsp: add_parameter octet=0x%02x\n", octet);
	}

	return (offset);
}

static guint
add_parameter_charset (proto_tree *tree, tvbuff_t *tvb, guint offset, guint startOffset)
{
	guint octet = tvb_get_guint8 (tvb, offset);
	if (octet < 31)
	{
		offset += octet+1;
		proto_tree_add_item (tree, hf_wsp_parameter_well_known_charset, tvb, startOffset+1, octet, bo_big_endian);
	}
	else if (octet & 0x80)
	{
		offset++;
		proto_tree_add_uint (tree, hf_wsp_parameter_well_known_charset, tvb, startOffset, offset-startOffset, (octet & 0x7F));
	}

	return offset;
}

static void
add_post_data (proto_tree *tree, tvbuff_t *tvb, guint contentType)
{
	guint offset = 0;
	guint variableStart = 0;
	guint variableEnd = 0;
	guint valueStart = 0;
	guint valueEnd = 0;
	guint8 peek = 0;
	proto_item *ti;
	
	/* VERIFY ti = proto_tree_add_item (tree, hf_wsp_post_data,tvb,offset,tvb_length_remaining(tvb, offset),bo_little_endian); */
	ti = proto_tree_add_item (tree, hf_wsp_post_data,tvb,offset,tvb_reported_length(tvb),bo_little_endian);

	if (contentType == 0x12)	/* URL Encoded data */
	{
		/* Iterate through post data */
		for (offset = 0; offset < tvb_reported_length (tvb); offset++)
		{
			peek = tvb_get_guint8 (tvb, offset);
			if (peek == '=')
			{
				variableEnd = offset;
				valueStart = offset+1;
			}
			else if (peek == '&')
			{
				if (variableEnd > 0)
				{
					add_post_variable (ti, tvb, variableStart, variableEnd, valueStart, offset);
				}
				variableStart = offset+1;
				variableEnd = 0;
				valueStart = 0;
				valueEnd = 0;
			}
		}

		/* See if there's outstanding data */
		if (variableEnd > 0)
		{
			add_post_variable (ti, tvb, variableStart, variableEnd, valueStart, offset);
		}
	}
}

static void
add_post_variable (proto_tree *tree, tvbuff_t *tvb, guint variableStart, guint variableEnd, guint valueStart, guint valueEnd)
{
	int variableLength = variableEnd-variableStart;
	int valueLength = 0;
	char *variableBuffer;
	char *valueBuffer;

	variableBuffer = g_malloc (variableLength+1);
	strncpy (variableBuffer, tvb_get_ptr (tvb, variableStart, variableLength), variableLength);
	variableBuffer[variableLength] = 0;

	if (valueEnd < valueStart)
	{
		valueBuffer = g_malloc (1);
		valueBuffer[0] = 0;
		valueEnd = valueStart;
	}
	else
	{
		valueLength = valueEnd-valueStart;
		valueBuffer = g_malloc (valueLength+1);
		strncpy (valueBuffer, tvb_get_ptr (tvb, valueStart, valueLength), valueLength);
		valueBuffer[valueLength] = 0;
	}

	/* Check for variables with no value */
	if (valueStart >= tvb_reported_length (tvb))
	{
		valueStart = tvb_reported_length (tvb);
		valueEnd = valueStart;
	}
	valueLength = valueEnd-valueStart;

	proto_tree_add_text (tree, tvb, variableStart, valueEnd-variableStart, "%s: %s", variableBuffer, valueBuffer);

	g_free (variableBuffer);
	g_free (valueBuffer);
}

/* Register the protocol with Ethereal */
void
proto_register_wsp(void)
{                 

/* Setup list of header fields */
	static hf_register_info hf[] = {
		{ &hf_wsp_header_tid,
			{ 	"Transmission ID",           
				"wsp.TID",
				 FT_UINT8, BASE_HEX, NULL, 0x00,
				"Transmission ID", HFILL
			}
		},
		{ &hf_wsp_header_pdu_type,
			{ 	"PDU Type",           
				"wsp.pdu_type",
				 FT_UINT8, BASE_HEX, VALS( vals_pdu_type ), 0x00,
				"PDU Type", HFILL
			}
		},
		{ &hf_wsp_version_major,
			{ 	"Version (Major)",           
				"wsp.version.major",
				 FT_UINT8, BASE_DEC, NULL, 0xF0,
				"Version (Major)", HFILL
			}
		},
		{ &hf_wsp_version_minor,
			{ 	"Version (Minor)",           
				"wsp.version.minor",
				 FT_UINT8, BASE_DEC, NULL, 0x0F,
				"Version (Minor)", HFILL
			}
		},
		{ &hf_wsp_capability_length,
			{ 	"Capability Length",           
				"wsp.capability.length",
				 FT_UINT32, BASE_DEC, NULL, 0x00,
				"Capability Length", HFILL
			}
		},
		{ &hf_wsp_header_length,
			{ 	"Headers Length",           
				"wsp.headers_length",
				 FT_UINT32, BASE_DEC, NULL, 0x00,
				"Headers Length", HFILL
			}
		},
		{ &hf_wsp_capabilities_section,
			{ 	"Capabilities",           
				"wsp.capabilities",
				 FT_NONE, BASE_DEC, NULL, 0x00,
				"Capabilities", HFILL
			}
		},
		{ &hf_wsp_headers_section,
			{ 	"Headers",           
				"wsp.headers",
				 FT_NONE, BASE_DEC, NULL, 0x00,
				"Headers", HFILL
			}
		},
		{ &hf_wsp_header,
			{ 	"Header",           
				"wsp.headers.header",
				 FT_NONE, BASE_DEC, NULL, 0x00,
				"Header", HFILL
			}
		},
		{ &hf_wsp_header_uri_len,
			{ 	"URI Length",           
				"wsp.uri_length",
				 FT_UINT32, BASE_DEC, NULL, 0x00,
				"URI Length", HFILL
			}
		},
		{ &hf_wsp_header_uri,
			{ 	"URI",           
				"wsp.uri",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"URI", HFILL
			}
		},
		{ &hf_wsp_server_session_id,
			{ 	"Server Session ID",           
				"wsp.server.session_id",
				 FT_UINT32, BASE_DEC, NULL, 0x00,
				"Server Session ID", HFILL
			}
		},
		{ &hf_wsp_header_status,
			{ 	"Status",           
				"wsp.reply.status",
				 FT_UINT8, BASE_HEX, VALS( vals_status ), 0x00,
				"Status", HFILL
			}
		},
		{ &hf_wsp_content_type,
			{ 	"Content Type",           
				"wsp.content_type.type",
				 FT_UINT8, BASE_HEX, VALS ( vals_content_types ), 0x00,
				"Content Type", HFILL
			}
		},
		{ &hf_wsp_parameter_well_known_charset,
			{ 	"Charset",           
				"wsp.content_type.parameter.charset",
				 FT_UINT16, BASE_HEX, VALS ( vals_character_sets ), 0x00,
				"Charset", HFILL
			}
		},
		{ &hf_wsp_reply_data,
			{ 	"Data",           
				"wsp.reply.data",
				 FT_NONE, BASE_NONE, NULL, 0x00,
				"Data", HFILL
			}
		},
		{ &hf_wsp_header_shift_code,
			{ 	"Shift code",           
				"wsp.header.shift",
				 /*FT_NONE, BASE_DEC, NULL, 0x00,*/
				 FT_UINT8, BASE_HEX, NULL, 0x00,
				"Shift code", HFILL
			}
		},
		{ &hf_wsp_header_accept,
			{ 	"Accept",           
				"wsp.header.accept",
				 /*FT_NONE, BASE_DEC, NULL, 0x00,*/
				 FT_UINT8, BASE_HEX, VALS ( vals_content_types ), 0x00,
				"Accept", HFILL
			}
		},
		{ &hf_wsp_header_accept_str,
			{ 	"Accept",           
				"wsp.header.accept.string",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Accept", HFILL
			}
		},
		{ &hf_wsp_header_accept_charset,
			{ 	"Accept-Charset",           
				"wsp.header.accept_charset",
				 FT_UINT16, BASE_HEX, VALS ( vals_character_sets ), 0x00,
				"Accept-Charset", HFILL
			}
		},
		{ &hf_wsp_header_accept_charset_str,
			{ 	"Accept-Charset",           
				"wsp.header.accept_charset.string",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Accept-Charset", HFILL
			}
		},
		{ &hf_wsp_header_accept_language,
			{ 	"Accept-Language",           
				"wsp.header.accept_language",
				 FT_UINT8, BASE_HEX, VALS ( vals_languages ), 0x00,
				"Accept-Language", HFILL
			}
		},
		{ &hf_wsp_header_accept_language_str,
			{ 	"Accept-Language",           
				"wsp.header.accept_language.string",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Accept-Language", HFILL
			}
		},
		{ &hf_wsp_header_accept_ranges,
			{ 	"Accept-Ranges",           
				"wsp.header.accept_ranges",
				 FT_UINT8, BASE_HEX, VALS ( vals_accept_ranges ), 0x00,
				"Accept-Ranges", HFILL
			}
		},
		{ &hf_wsp_header_age,
			{ 	"Age",           
				"wsp.header.age",
				 FT_UINT32, BASE_DEC, NULL, 0x00,
				"Age", HFILL
			}
		},
		{ &hf_wsp_header_cache_control,
			{ 	"Cache-Control",           
				"wsp.header.cache_control",
				 FT_UINT8, BASE_HEX, VALS ( vals_cache_control ), 0x00,
				"Cache-Control", HFILL
			}
		},
		{ &hf_wsp_header_content_length,
			{ 	"Content-Length",           
				"wsp.header.content_length",
				 FT_UINT32, BASE_DEC, NULL, 0x00,
				"Content-Length", HFILL
			}
		},
		{ &hf_wsp_header_date,
			{ 	"Date",           
				"wsp.header.date",
				 FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
				"Date", HFILL
			}
		},
		{ &hf_wsp_header_etag,
			{ 	"Etag",           
				"wsp.header.etag",
				 /*FT_NONE, BASE_DEC, NULL, 0x00,*/
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Etag", HFILL
			}
		},
		{ &hf_wsp_header_expires,
			{ 	"Expires",           
				"wsp.header.expires",
				 FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
				"Expires", HFILL
			}
		},
		{ &hf_wsp_header_last_modified,
			{ 	"Last-Modified",           
				"wsp.header.last_modified",
				 FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
				"Last-Modified", HFILL
			}
		},
		{ &hf_wsp_header_location,
			{ 	"Location",           
				"wsp.header.location",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Location", HFILL
			}
		},
		{ &hf_wsp_header_if_modified_since,
			{ 	"If-Modified-Since",           
				"wsp.header.if_modified_since",
				 FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
				"If-Modified-Since", HFILL
			}
		},
		{ &hf_wsp_header_server,
			{ 	"Server",           
				"wsp.header.server",
				 /*FT_NONE, BASE_DEC, NULL, 0x00,*/
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Server", HFILL
			}
		},
		{ &hf_wsp_header_transfer_encoding,
			{ 	"Transfer Encoding",           
				"wsp.header.transfer_enc",
				 /*FT_NONE, BASE_DEC, NULL, 0x00,*/
				 FT_UINT8, BASE_HEX, VALS ( vals_transfer_encoding ), 0x00,
				"Transfer Encoding", HFILL
			}
		},
		{ &hf_wsp_header_transfer_encoding_str,
			{ 	"Transfer Encoding",           
				"wsp.header.transfer_enc_str",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Transfer Encoding", HFILL
			}
		},
		{ &hf_wsp_header_user_agent,
			{ 	"User-Agent",           
				"wsp.header.user_agent",
				 /*FT_NONE, BASE_DEC, NULL, 0x00,*/
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"User-Agent", HFILL
			}
		},
		{ &hf_wsp_header_via,
			{ 	"Via",           
				"wsp.header.via",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Via", HFILL
			}
		},
		{ &hf_wsp_header_application_header,
			{ 	"Application Header",           
				"wsp.header.application_header",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Application Header", HFILL
			}
		},
		{ &hf_wsp_header_application_value,
			{ 	"Application Header Value",           
				"wsp.header.application_header.value",
				 FT_STRING, BASE_NONE, NULL, 0x00,
				"Application Header Value", HFILL
			}
		},
		{ &hf_wsp_header_x_wap_tod,
			{ 	"X-WAP.TOD",           
				"wsp.header.x_wap_tod",
				 FT_ABSOLUTE_TIME, BASE_NONE, NULL, 0x0,
				"X-WAP.TOD", HFILL
			}
		},
		{ &hf_wsp_capabilities_client_SDU,
			{	"Client SDU",
				"wsp.capabilities.client_SDU",
				 FT_UINT8, BASE_DEC, NULL, 0x00,
				"Client SDU", HFILL
			}
		},
		{ &hf_wsp_capabilities_server_SDU,
			{	"Server SDU",
				"wsp.capabilities.server_SDU",
				 FT_UINT8, BASE_DEC, NULL, 0x00,
				"Server SDU", HFILL
			}
		},
		{ &hf_wsp_capabilities_protocol_opt,
			{	"Protocol Options",
				"wsp.capabilities.protocol_opt",
				 FT_STRING, BASE_HEX, NULL, 0x00,
				"Protocol Options", HFILL
			}
		},
		{ &hf_wsp_capabilities_method_MOR,
			{	"Method MOR",
				"wsp.capabilities.method_mor",
				 FT_UINT8, BASE_DEC, NULL, 0x00,
				"Method MOR", HFILL
			}
		},
		{ &hf_wsp_capabilities_push_MOR,
			{	"Push MOR",
				"wsp.capabilities.push_mor",
				 FT_UINT8, BASE_DEC, NULL, 0x00,
				"Push MOR", HFILL
			}
		},
		{ &hf_wsp_capabilities_extended_methods,
			{	"Extended Methods",
				"wsp.capabilities.extend_methods",
				 FT_STRING, BASE_HEX, NULL, 0x00,
				"Extended Methods", HFILL
			}
		},
		{ &hf_wsp_capabilities_header_code_pages,
			{	"Header Code Pages",
				"wsp.capabilities.code_pages",
				 FT_STRING, BASE_HEX, NULL, 0x00,
				"Header Code Pages", HFILL
			}
		},
		{ &hf_wsp_capabilities_aliases,
			{	"Aliases",
				"wsp.capabilities.aliases",
				 FT_UINT8, BASE_HEX, NULL, 0x00,
				"Aliases", HFILL
			}
		},
		{ &hf_wsp_post_data,
			{ 	"Post Data",           
				"wsp.post.data",
				 FT_NONE, BASE_NONE, NULL, 0x00,
				"Post Data", HFILL
			}
		},
		{ &hf_wsp_redirect_flags,
			{ 	"Flags",
				"wsp.redirect_flags",
				 FT_UINT8, BASE_HEX, NULL, 0x00,
				"Redirect Flags", HFILL
			}
		},
		{ &hf_wsp_redirect_permanent,
			{ 	"Permanent Redirect",
				"wsp.redirect_flags.permanent",
				 FT_BOOLEAN, 8, TFS(&yes_no_truth), PERMANENT_REDIRECT,
				"Permanent Redirect", HFILL
			}
		},
		{ &hf_wsp_redirect_reuse_security_session,
			{ 	"Reuse Security Session",
				"wsp.redirect_flags.reuse_security_session",
				 FT_BOOLEAN, 8, TFS(&yes_no_truth), REUSE_SECURITY_SESSION,
				"Permanent Redirect", HFILL
			}
		},
		{ &hf_wsp_redirect_afl,
			{ 	"Flags/Length",
				"wsp.redirect_afl",
				 FT_UINT8, BASE_HEX, NULL, 0x00,
				"Redirect Address Flags/Length", HFILL
			}
		},
		{ &hf_wsp_redirect_afl_bearer_type_included,
			{ 	"Bearer Type Included",
				"wsp.redirect_afl.bearer_type_included",
				 FT_BOOLEAN, 8, TFS(&yes_no_truth), BEARER_TYPE_INCLUDED,
				"Redirect Address bearer type included", HFILL
			}
		},
		{ &hf_wsp_redirect_afl_port_number_included,
			{ 	"Port Number Included",
				"wsp.redirect_afl.port_number_included",
				 FT_BOOLEAN, 8, TFS(&yes_no_truth), PORT_NUMBER_INCLUDED,
				"Redirect Address port number included", HFILL
			}
		},
		{ &hf_wsp_redirect_afl_address_len,
			{ 	"Address Len",
				"wsp.redirect_afl.address_len",
				 FT_UINT8, BASE_DEC, NULL, ADDRESS_LEN,
				"Redirect Address Length", HFILL
			}
		},
		{ &hf_wsp_redirect_bearer_type,
			/*
			 * XXX - the values are specified in the WDP
			 * specification.
			 */
			{ 	"Bearer Type",
				"wsp.redirect_bearer_type",
				 FT_UINT8, BASE_DEC, NULL, 0x0,
				"Redirect Bearer Type", HFILL
			}
		},
		{ &hf_wsp_redirect_port_num,
			{ 	"Port Number",
				"wsp.redirect_port_num",
				 FT_UINT16, BASE_DEC, NULL, 0x0,
				"Redirect Port Number", HFILL
			}
		},
		{ &hf_wsp_redirect_ipv4_addr,
			{ 	"IP Address",
				"wsp.redirect_ipv4_addr",
				 FT_IPv4, BASE_NONE, NULL, 0x0,
				"Redirect Address (IP)", HFILL
			}
		},
		{ &hf_wsp_redirect_addr,
			{ 	"Address",
				"wsp.redirect_addr",
				 FT_BYTES, BASE_NONE, NULL, 0x0,
				"Redirect Address", HFILL
			}
		},
	};
	
/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_wsp,
		&ett_header,
		&ett_headers,
		&ett_capabilities,
		&ett_content_type,
		&ett_redirect_flags,
		&ett_redirect_afl,
	};

/* Register the protocol name and description */
	proto_wsp = proto_register_protocol(
		"Wireless Session Protocol",   	/* protocol name for use by ethereal */ 
		"WSP",                          /* short version of name */
		"wap-wsp"                   	/* Abbreviated protocol name, should Match IANA 
						    < URL:http://www.isi.edu/in-notes/iana/assignments/port-numbers/ >
						  */
	);

/* Required function calls to register the header fields and subtrees used  */
	proto_register_field_array(proto_wsp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("wsp", dissect_wsp_fromwap, proto_wsp);
};

void
proto_reg_handoff_wsp(void)
{
	/*
	 * Get a handle for the WMLC dissector
	 */
	wmlc_handle = find_dissector("wmlc");	/* Coming soon :) */

	/* Only connection-less WSP has no previous handler */
	dissector_add("udp.port", UDP_PORT_WSP, dissect_wsp_fromudp, proto_wsp);

	/* This dissector is also called from the WTP and WTLS dissectors */
}

static gint get_long_integer (tvbuff_t *tvb, guint *offset, guint offset_end, guint *value)
{
	guint valueLength;

	valueLength = tvb_get_guint8(tvb,*offset);
	if ((valueLength+*offset) > offset_end)
	{
		*value = 0;
		return (-1);
	}
	(*offset)++;
	switch (valueLength)
	{
		case 1:
			*value = tvb_get_guint8(tvb,*offset);
			break;
		case 2:
			*value = tvb_get_ntohs(tvb,*offset);
			break;
		case 3:
		        *value = tvb_get_ntoh24(tvb,*offset);
			break;
		case 4:
			*value = tvb_get_ntohl(tvb,*offset);
			break;
		default:
		        /* TODO: Need to read peek octets */
		        *value = 0;
		        fprintf (stderr, "dissect_wsp: get_long_integer size %d NYI\n", valueLength);
	        	break;
	}
	(*offset) += valueLength;
	return 0;
}
