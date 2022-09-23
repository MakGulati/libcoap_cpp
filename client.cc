/* minimal CoAP client
 *
 * Copyright (C) 2018-2021 Olaf Bergmann <bergmann@tzi.org>
 */

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "common.hh"
#include "nanocbor/nanocbor.h"

static int have_response = 0;

int main(void)
{
  coap_context_t *ctx = nullptr;
  coap_session_t *session = nullptr;
  coap_address_t dst;
  coap_pdu_t *pdu = nullptr;
  coap_pdu_t *pdu1 = nullptr;
  int result = EXIT_FAILURE;
  ;
  uint8_t payload_buffer[128];
  nanocbor_encoder_t enc;
  float m0_local = 3.512735;
  float m1_local = 9.285732;
  float m2_local = 4.994352;
  float b_local = 1.366232;
  nanocbor_encoder_init(&enc, payload_buffer, sizeof(payload_buffer));
  nanocbor_fmt_array(&enc, 4);
  nanocbor_fmt_float(&enc, m0_local);
  nanocbor_fmt_float(&enc, m1_local);
  nanocbor_fmt_float(&enc, m2_local);
  nanocbor_fmt_float(&enc, b_local);
  size_t payload_len = nanocbor_encoded_len(&enc);

  coap_startup();

  /* Set logging level */
  coap_set_log_level(LOG_WARNING);

  /* resolve destination address where server should be sent */
  if (resolve_address("2001:db8::1", "5683", &dst) < 0)
  {
    coap_log(LOG_CRIT, "failed to resolve address\n");
    goto finish;
  }

  /* create CoAP context and a client session */
  if (!(ctx = coap_new_context(nullptr)))
  {
    coap_log(LOG_EMERG, "cannot create libcoap context\n");
    goto finish;
  }
  /* Support large responses */
  coap_context_set_block_mode(ctx,
                              COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);

  if (!(session = coap_new_client_session(ctx, nullptr, &dst,
                                          COAP_PROTO_UDP)))
  {
    coap_log(LOG_EMERG, "cannot create client session\n");
    goto finish;
  }

  /* coap_register_response_handler(ctx, response_handler); */
  coap_register_response_handler(ctx, [](auto, auto,
                                         const coap_pdu_t *received,
                                         auto)
                                 {
                                        have_response = 1;
                                        coap_show_pdu(LOG_WARNING, received);
                                        return COAP_RESPONSE_OK; });
  /* construct CoAP message */
  pdu = coap_pdu_init(COAP_MESSAGE_CON,
                      COAP_REQUEST_CODE_GET,
                      coap_new_message_id(session),
                      coap_session_max_pdu_size(session));
  pdu1 = coap_pdu_init(COAP_MESSAGE_CON,
                       COAP_REQUEST_CODE_PUT,
                       coap_new_message_id(session),
                       coap_session_max_pdu_size(session));
  // std::cout << coap_session_max_pdu_size(session) << std::endl;

  if (!pdu)
  {
    coap_log(LOG_EMERG, "cannot create PDU\n");
    goto finish;
  }

  /* add a Uri-Path option */
  coap_add_option(pdu, COAP_OPTION_URI_PATH, 14,
                  reinterpret_cast<const uint8_t *>("cli/stats_test"));
  coap_add_option(pdu1, COAP_OPTION_URI_PATH, 14,
                  reinterpret_cast<const uint8_t *>("cli/stats_test"));
  // GET REQUEST
  // coap_show_pdu(LOG_WARNING, pdu);
  /* and send the PDU */
  coap_send(session, pdu);
  // coap_show_pdu(LOG_WARNING, pdu);

  // PUT REQUEST
  coap_add_data(pdu1, payload_len, payload_buffer);

  coap_show_pdu(LOG_WARNING, pdu1);
  /* and send the PDU */
  coap_send(session, pdu1);

  pdu = coap_pdu_init(COAP_MESSAGE_CON,
                      COAP_REQUEST_CODE_GET,
                      coap_new_message_id(session),
                      coap_session_max_pdu_size(session));
  // GET REQUEST
  coap_show_pdu(LOG_WARNING, pdu);
  /* and send the PDU */
  coap_send(session, pdu);
  coap_show_pdu(LOG_WARNING, pdu);

  while (have_response == 0)
    coap_io_process(ctx, COAP_IO_WAIT);

  result = EXIT_SUCCESS;
finish:

  coap_session_release(session);
  coap_free_context(ctx);
  coap_cleanup();

  return result;
}