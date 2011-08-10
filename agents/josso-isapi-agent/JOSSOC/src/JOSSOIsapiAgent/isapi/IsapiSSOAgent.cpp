#define _CRTDBG_MAP_ALLOC
#include <iostream>
#include <crtdbg.h>

#include <JOSSOIsapiAgent/isapi/IsapiSSOAgent.hpp>
#include <JOSSOIsapiAgent/isapi/IsapiAgentConfig.hpp>

#include "JOSSOIsapiAgent/util/simpleini/SimpleIni.h"
#include "JOSSOIsapiAgent/util/StringUtil.hpp"

#include "JOSSOIsapiAgent/agent/autologin/DefaultAutomaticLoginStrategy.hpp"
#include "JOSSOIsapiAgent/agent/autologin/UrlBasedAutomaticLoginStrategy.hpp"
#include "JOSSOIsapiAgent/agent/autologin/BotAutomaticLoginStrategy.hpp"

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif


// Initialize some constants

const char *IsapiSSOAgent::JOSSO_REGISTRY_LOCATION = "Software\\Atricore\\JOSSO Isapi Agent\\1.8";

const char *IsapiSSOAgent::JOSSO_LOG_FILE = "LogFile";

const char *IsapiSSOAgent::JOSSO_LOG_LEVEL = "LogLevel";

const char *IsapiSSOAgent::JOSSO_AGENT_CONFIG = "AgentConfigFile";

const char *IsapiSSOAgent::JOSSO_EXTENSION_URI = "ExtensionUri";

const char *IsapiSSOAgent::W3SVC_REGISTRY_KEY = "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters";

/**
 * 
 **/
bool IsapiSSOAgent::start() {

	bool ok = true;

	syslog(JK_LOG_INFO_LEVEL, "Starting JOSSO Isapi Agent");

	// Initialize JOSSO Isapi Agent
	ok = ok & AbstractSSOAgent::start();

	return ok;

}

bool IsapiSSOAgent::stop() {
	bool ok = true;
	
	if(!jk_close_file_logger(&logger))
		ok = false;

	ok = ok && AbstractSSOAgent::stop();

	return ok;
}
bool IsapiSSOAgent::requestLogin(SSOAgentRequest * req, SSOAgentResponse * res, PartnerAppConfig * appCfg, bool optional) {

	string agentLoginUrl (this->getExtensionUri());
	if (optional)
		// If login fails the gateway will silently return to the back_to resource.
		agentLoginUrl.append("?josso_login_optional");
	else
		agentLoginUrl.append("?josso_login");

	agentLoginUrl.append("&josso_partnerapp_id=");
	agentLoginUrl.append(appCfg->getPartnerAppId());

	string &path = req->getPath();
	string encodedPath = StringUtil::encode64(path);
	string encodedSplashResource = StringUtil::encode64(appCfg->getSplashResource());

	jk_log(this->logger, JK_LOG_TRACE, "Encoded PATH %s", encodedPath.c_str());
	jk_log(this->logger, JK_LOG_TRACE, "Encoded SPLASH RESOURCE %s", encodedSplashResource.c_str());
					
	res->setCookie("JOSSO_RESOURCE", encodedPath, "/");
	res->setCookie("JOSSO_SPLASH_RESOURCE", encodedSplashResource, "/");
	
	res->sendRedirect(agentLoginUrl);

	return TRUE;

}

char * IsapiSSOAgent::getExtensionUri() {
	return ((IsapiAgentConfig *)this->agentConfig)->extensionUri;
}



string IsapiSSOAgent::buildGwyLoginUrl(SSOAgentRequest *req) {

	string url (getGwyLoginUrl());

	if (url.find("?") == string::npos)
		url.append("?");
	else
		url.append("&");

	url.append("&josso_back_to=");

	string host = req->getServerVariable("HTTP_HOST", MAX_HEADER_SIZE);
	string https = req->getServerVariable("HTTPS", MAX_HEADER_SIZE);

	url.append(https == "on" || https == "ON" ? "https://" : "http://");
	url.append(host);
	url.append(getExtensionUri());
	url.append("%3Fjosso_security_check&amp;");
	url.append("josso_partnerapp_host=");
	url.append(host.c_str());

	return url;

}

string IsapiSSOAgent::buildGwyLogoutUrl(SSOAgentRequest *req) {

	string url (getGwyLogoutUrl());

	if (url.find("?") == string::npos)
		url.append("?");
	else
		url.append("&");

	url.append("josso_back_to=");

	string host = req->getServerVariable("HTTP_HOST", MAX_HEADER_SIZE);
	string https = req->getServerVariable("HTTPS", MAX_HEADER_SIZE);

	url.append(https == "on" || https == "ON" ? "https://" : "http://");
	url.append(host);
	url.append(getExtensionUri());
	url.append("%3Fjosso_security_check&amp;");
	url.append("josso_partnerapp_host=");
	url.append(host.c_str());

	return url;

}

SSOAgentRequest *IsapiSSOAgent::initIsapiFilterRequest(HTTP_FILTER_CONTEXT *pfc, 
										   DWORD NotificationType, 
										   VOID* pvNotification) {

	jk_log(logger, JK_LOG_TRACE, "Creating FilterAgentRequest instance");
	SSOAgentRequest * req = new FilterAgentRequest(pfc, NotificationType, pvNotification, logger);
	jk_log(logger, JK_LOG_TRACE, "Creating FilterAgentRequest instance ... OK");

	return req;
}

SSOAgentResponse *IsapiSSOAgent::initIsapiFilterResponse(HTTP_FILTER_CONTEXT *pfc, 
										   DWORD NotificationType, 
										   VOID* pvNotification) {

	jk_log(logger, JK_LOG_TRACE, "Creating FilterAgentResponse instance");
	SSOAgentResponse * res = new FilterAgentResponse(pfc, NotificationType, pvNotification, logger);
	jk_log(logger, JK_LOG_TRACE, "Creating FilterAgentResponse instance ... OK");

	return res;
}

SSOAgentResponse *IsapiSSOAgent::initIsapiExtensionResponse(LPEXTENSION_CONTROL_BLOCK lpEcb) {
	jk_log(logger, JK_LOG_TRACE, "Creating ExtensionAgentResponse instance");
	SSOAgentResponse * res = new ExtensionAgentResponse(lpEcb, logger);
	jk_log(logger, JK_LOG_TRACE, "Creating ExtensionAgentResponse instance ... OK");

	return res;
}

SSOAgentRequest *IsapiSSOAgent::initIsapiExtensionRequest(LPEXTENSION_CONTROL_BLOCK lpEcb) {
	jk_log(logger, JK_LOG_TRACE, "Creating ExtensionAgentRequest instance");
	SSOAgentRequest * req = new ExtensionAgentRequest(lpEcb, logger);
	jk_log(logger, JK_LOG_TRACE, "Creating ExtensionAgentRequest instance ... OK");

	return req;
}

const char *IsapiSSOAgent::getRequester(SSOAgentRequest *req) {
	string originalResource = req->getPath();
	if (originalResource.empty() || !originalResource.find(this->getExtensionUri())) {
		originalResource = req->getCookie("JOSSO_RESOURCE");
		originalResource = StringUtil::decode64(originalResource);
	}
	if (!originalResource.empty()) {
		PartnerAppConfig *appCfg = getPartnerAppConfig(originalResource);
		if (appCfg != NULL) {
			return appCfg->getPartnerAppId();
		}
	}
	return NULL;
}

// ---------------------------------------------------------------------------------------------

AgentConfig *IsapiSSOAgent::createAgentConfig() {
	return new IsapiAgentConfig();
}

bool IsapiSSOAgent::configureAgent(AgentConfig *config) {

    bool ok = true;
    HKEY hkey;
	
	syslog(JK_LOG_TRACE_LEVEL, "Starting JOSSO Isapi Agent Configuration ... ");

	IsapiAgentConfig *cfg = (IsapiAgentConfig *) config; 
	if (cfg == NULL) {
		syslog(JK_LOG_ERROR_LEVEL, "Configuration is NULL");
	}
	
	cfg->logLevel = JK_LOG_INFO_LEVEL;

	syslog(JK_LOG_DEBUG_LEVEL, "Opening Windows Registry [%s]", JOSSO_REGISTRY_LOCATION);

    long rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, JOSSO_REGISTRY_LOCATION,
                           (DWORD)0, KEY_READ, &hkey);
    if (ERROR_SUCCESS != rc) {
		syslog(JK_LOG_ERROR_LEVEL, "Cannot open Windows Registry, return code : %d" + rc);
        return false;
    }

	// Log File
    ok = ok && getRegistryValue(hkey, JOSSO_LOG_FILE, cfg->logFile, sizeof(cfg->logFile));
	syslog(JK_LOG_DEBUG_LEVEL, "%s [%s]", JOSSO_LOG_FILE, cfg->logFile);

    // Log Level (optional)
	char tmpbuf[MAX_PATH];
    if (getRegistryValue(hkey, JOSSO_LOG_LEVEL, tmpbuf, sizeof(tmpbuf))) {
        cfg->logLevel = jk_parse_log_level(tmpbuf);
		syslog(JK_LOG_DEBUG_LEVEL, "%s [%d] [%s]", JOSSO_LOG_LEVEL, cfg->logLevel, tmpbuf);
    }
	
	// Extension URI
    ok = ok && getRegistryValue(hkey, JOSSO_EXTENSION_URI, cfg->extensionUri, sizeof(cfg->extensionUri));
	syslog(JK_LOG_DEBUG_LEVEL, "%s [%s]", JOSSO_EXTENSION_URI, cfg->extensionUri);

	// Agent configuraiton file 
    ok = ok && getRegistryValue(hkey, JOSSO_AGENT_CONFIG, cfg->agentConfigFile, sizeof(cfg->agentConfigFile));
	syslog(JK_LOG_DEBUG_LEVEL, "%s [%s]", JOSSO_AGENT_CONFIG, cfg->agentConfigFile);


    RegCloseKey(hkey);

	// Now that we have done some initialization , call our parent class
	AbstractSSOAgent::configureAgent(cfg);
    
	// Load automatic login strategies
	
	bool a_bIsUtf8 = false;
    bool a_bUseMultiKey = true;
    bool a_bUseMultiLine = true;

	// Agent config INI file
	CSimpleIniA ini(a_bIsUtf8, a_bUseMultiKey, a_bUseMultiLine);
	SI_Error rc2 = ini.LoadFile(cfg->agentConfigFile);
	if (rc2 < 0) {
		syslog(JK_LOG_ERROR_LEVEL, "Cannot read agent configuartion file %s", cfg->agentConfigFile);
		return false;
	}

	CSimpleIniA::TNamesDepend sections;
    ini.GetAllSections(sections);

	CSimpleIniA::TNamesDepend::const_iterator i;
    for (i = sections.begin(); i != sections.end(); ++i) {
		
		const char *section = i->pItem;

		if (section != NULL &&
			strlen(section) >= 24 &&
			strncmp(section, "automatic-login-strategy", 24) == 0) {

			const char *strategy = ini.GetValue(section, "strategy", NULL );
			if (strategy == NULL) {
				syslog(JK_LOG_DEBUG_LEVEL, "'strategy' not found in '%s' section", section);
			}

			const char *mode = ini.GetValue(section, "mode", NULL );
			if (mode == NULL) {
				syslog(JK_LOG_WARNING_LEVEL, "'mode' not found in '%s' section", section);
			}

			if (strategy != NULL && mode != NULL) {

				if (!strcmp(strategy, JOSSO_DEFAULT_AUTH_LOGIN_STRATEGY)) {
					DefaultAutomaticLoginStrategy *defaultStrategy = new DefaultAutomaticLoginStrategy(mode);
					defaultStrategy->setSSOAgent(this);
					const char *ignoredReferes = ini.GetValue(section, "ignored-referers", NULL );
					if (ignoredReferes == NULL) {
						syslog(JK_LOG_WARNING_LEVEL, "'ignored-referers' not found in '%s' section", section);
					} else {
						string referes (ignoredReferes);
						StringUtil::tokenize(referes, defaultStrategy->ignoredReferers, ",");
					}
					this->automaticStrategies.push_back(defaultStrategy);
				} else if (!strcmp(strategy, JOSSO_URLBASED_AUTH_LOGIN_STRATEGY)) {
					const char *urlPatterns = ini.GetValue(section, "url-patterns", NULL );
					if (urlPatterns == NULL) {
						syslog(JK_LOG_WARNING_LEVEL, "'url-patterns' not found in '%s' section", section);
					} else {
						UrlBasedAutomaticLoginStrategy *urlBasedStrategy = new UrlBasedAutomaticLoginStrategy(mode);
						urlBasedStrategy->setSSOAgent(this);
						string patterns (urlPatterns);
						StringUtil::tokenize(patterns, urlBasedStrategy->urlPatterns, ",");
						this->automaticStrategies.push_back(urlBasedStrategy);
					}
				} else if (!strcmp(strategy, JOSSO_BOT_AUTH_LOGIN_STRATEGY)) {
					const char *botsFile = ini.GetValue(section, "bots-file", NULL );
					if (botsFile == NULL) {
						syslog(JK_LOG_WARNING_LEVEL, "'bots-file' not found in '%s' section", section);
					} else {
						BotAutomaticLoginStrategy *botStrategy = new BotAutomaticLoginStrategy(mode);
						botStrategy->setSSOAgent(this);
						botStrategy->setBotsFile(botsFile);
						this->automaticStrategies.push_back(botStrategy);
					}
				}

			}

		}
    }

	if (this->automaticStrategies.empty()) {
		DefaultAutomaticLoginStrategy *defaultStrategy = new DefaultAutomaticLoginStrategy(JOSSO_AUTH_LOGIN_SUFFICIENT);
		defaultStrategy->setSSOAgent(this);
		this->automaticStrategies.push_back(defaultStrategy);
	}

    return ok;
}



bool IsapiSSOAgent::getIISInfo() {
    HKEY hkey;
    long rc;
    bool rv = false;

    int major = 0;
    int minor = 0;

    /* Retrieve the IIS version Major/Minor */
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      W3SVC_REGISTRY_KEY, (DWORD) 0, KEY_READ, &hkey);

    if (ERROR_SUCCESS != rc)
		return false;
	
	
	if (getRegistryNumber(hkey, "MajorVersion", &major)) {
		if (major > 4) {
    		if (getRegistryNumber(hkey, "MinorVersion", &minor) == JK_TRUE) {
	            rv = true;
				syslog(JK_LOG_DEBUG_LEVEL, "IIS Server version detected %d.%d", major, minor);
			}
		}
	}

	CloseHandle(hkey);

	
    return rv;
}

bool IsapiSSOAgent::getRegistryValue(HKEY hkey, const char *tag, char *b, DWORD sz)
{    
	DWORD type = 0;
    LONG lrc;

    sz = sz - 1; /* Reserve space for RegQueryValueEx to add null terminator */
    b[sz] = '\0'; /* Null terminate in case RegQueryValueEx doesn't */

    lrc = RegQueryValueEx(hkey, tag, (LPDWORD) 0, &type, (LPBYTE) b, &sz);
    if ((ERROR_SUCCESS != lrc) || (type != REG_SZ)) {
        return false;
    }

    return true;
}


bool IsapiSSOAgent::getRegistryNumber(HKEY hkey,
                                      const char *tag, int *val)
{
    DWORD type = 0;
    DWORD data = 0;
    DWORD sz   = sizeof(DWORD);
    LONG lrc;

    lrc = RegQueryValueEx(hkey, tag, (LPDWORD)0, &type, (LPBYTE)&data, &sz);
    if ((ERROR_SUCCESS != lrc) || (type != REG_DWORD)) {
        return false;
    }

    *val = (int)data;

    return true;
}
