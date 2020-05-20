#ifndef WUI_REQUEST_PARSER_H
#define WUI_REQUEST_PARSER_H

#include "http_client.h"
#include "httpd.h"

/*!*************************************************************************************************
* \brief Parses JSON requests from HTTP Server (local webpage) and sends commands in WUI thread
*
* \param [in] json - Unparsed JSON data structure
*
* \param [in] len - Unparsed JSON data structure length
*
* \retval 1 if successful, 0 if error occured
***************************************************************************************************/
uint32_t httpd_json_parser(char *json, uint32_t len);

/*!********************************************************************************************************
* \brief Devides replies on low level command and high level command and calls appropriate parsing function
*
* \param [in] reply - Unparsed reply string
*
* \param [in] reply_len - Unparsed reply string length
*
* \param [in] h_info_ptr - Pointer to reply's HTTP header information
*
* \return HTTP_COMMAND_STATUS - Returns specified status describing how parsing went
***********************************************************************************************************/

HTTPC_COMMAND_STATUS parse_http_reply(char *reply, uint32_t reply_len, httpc_header_info *h_info_ptr);

#endif // WUI_REQUEST_PARSER_H
