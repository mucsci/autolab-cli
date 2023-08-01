/*
 * Used to define secret credentials used by the program.
 *
 * User should duplicate this file into 'app_credentials.h' and fill out the
 * fields prior to building.
 * Warning: the resulting 'app_credentials.h' should not be tracked in version
 * control. This has already been setup in .gitignore.
 */

#ifndef AUTOLAB_APP_CREDENTIALS_H_
#define AUTOLAB_APP_CREDENTIALS_H_

/* The domain of the autolab service. Do NOT include the trailing slash.
 * Must use 'https' instead of 'http' in production.
 */
const std::string server_domain = "https://autolab.millersville.edu";

/* The client_id and client_secret generated when registering the app */
const std::string client_id = "c6552fcc3ae736a0a95833bef6b43207d2c71a4100b5c7927ae0e7a57e5e88c3";
const std::string client_secret = "bf9240c8c43a4ee6b26da2e4a4bfdad2d7fac2d07429399d7bb16baa3a62d728";

/* The redirect uri used when registering the app.
 * For clients that use device_flow, this should have been
 *   "<host>/device_flow_auth_cb"
 */
const std::string redirect_uri = "https://autolab.millersville.edu/device_flow_auth_cb";

/* Key and iv used to encrypt tokens.
 * Note: lengths of strings must be correct:
 *  - key: 256 bits, or 32 regular chars
 *  - iv : 128 bits, or 16 regular chars
 * For safety reasons, please keep these declarations as macros instead of
 * variables.
 */
#define crypto_key ((unsigned char *)"EnterYourVerySecretCryptoKeyHere")
#define crypto_iv  ((unsigned char *)"YourVerySecretIV")

#endif /* AUTOLAB_APP_CREDENTIALS_H_ */
