#include <stdio.h>
#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include "minhook/include/MinHook.h"
#endif

#define DEBUG

#define VER_MAJOR 3
#define VER_MINOR 0
#define VER_PATCH 0

struct FunctionBase;

struct FunctionBase_vtable {
  size_t _reserved[2];
  union {
    void *operator_fnPtr;
    void (*operator_fn)(struct FunctionBase *thisPtr);
  };
  size_t _reserved1[32];
};

struct FunctionBase {
  struct FunctionBase_vtable *vtable;
  size_t _reserved[4];
};

struct ProductBase_vtable {
  size_t _reserved[8];
};

struct ProductBase {
  struct ProductBase_vtable *vtable;
  size_t _reserved[1];
  const char *name;
  const char *vendor;
  size_t _reserved2[7];
  union {
    struct {
      bool isActivated;
      bool isFree;
      bool _reserved1;
      bool hasNoProductKey;
    };
    uint8_t activationFlags[4];
  };
};

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

static LPCTSTR _lifeCycle = "UNKNOWN";
#define LIFECYCLE(x) (_lifeCycle = x)

#define die() _die(__LINE__)
void _Noreturn _die();

#define SV_CHECKED_BASE 0x140000000

static uintptr_t fptrAddBase = 1;
static LPVOID getActivationStatusFPtr = (LPVOID)0; // usually (1.11.2) -> (LPVOID)(0x1404acf50 - SV_CHECKED_BASE)
static LPVOID voiceDBVerifyProductKeyFPtr = (LPVOID)0; // usually (1.11.2) -> (LPVOID)(0x1404a9ef0 - SV_CHECKED_BASE)
static LPVOID appVerifyProductKeyFPtr = (LPVOID)0; // usually (1.11.2) -> (LPVOID)(0x1404ac200 - SV_CHECKED_BASE)
static LPVOID setOkFlagFPtr = (LPVOID)0; // usually (1.11.2) -> (LPVOID)(0x1400573b0 - SV_CHECKED_BASE)

static uint8_t getActivationStatusFSig_[] = {
  0x4c, 0x89, 0x44, 0x24, 0x18, 0x48, 0x89, 0x54, 0x24, 0x10, 0x53, 0x55, 0x56, 0x57, 0x41, 0x56,
  0x48, 0x83, 0xec, 0x60, 0x49, 0x8b, 0xf8, 0x48, 0x8b, 0xf2, 0x48, 0x8b, 0xd9, 0xe8, 255 , 255 ,
};
static uint8_t *getActivationStatusFSig = getActivationStatusFSig_;
static size_t getActivationStatusFSigSize = sizeof(getActivationStatusFSig_);

static uint8_t voiceDBVerifyProductKeyFSig_[] = {
  0x48, 0x89, 0x5c, 0x24, 0x20, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
  0x48, 0x8d, 0xac, 0x24, 0x00, 0xff, 0xff, 0xff, 0x48, 0x81, 0xec, 0x00, 0x02, 0x00, 0x00, 255 ,
  255 , 255 , 255 , 255 , 255 , 0x00, 0x48, 0x33, 0xc4, 0x48, 0x89, 0x85, 0xf0, 0x00, 0x00, 0x00,
  0x4d, 0x8b, 0xf0, 0x4c, 0x8b, 0xfa, 0x48, 0x8b, 0xf9, 0x48, 0x89, 0x54, 0x24, 0x48, 0x4c, 0x89,
  0x44, 0x24, 0x50, 0x45, 0x33, 0xed, 0x44, 0x89, 0x6c, 0x24, 0x38, 0xe8, 255 , 255 , 255 , 255 ,
};
static uint8_t *voiceDBVerifyProductKeyFSig = voiceDBVerifyProductKeyFSig_;
static size_t voiceDBVerifyProductKeyFSigSize = sizeof(voiceDBVerifyProductKeyFSig_);

static uint8_t appVerifyProductKeyFSig_[] = {
  0x48, 0x89, 0x5c, 0x24, 0x20, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
  0x48, 0x8d, 0xac, 0x24, 0x00, 0xff, 0xff, 0xff, 0x48, 0x81, 0xec, 0x00, 0x02, 0x00, 0x00, 255 ,
  255 , 255 , 255 , 255 , 255 , 0x00, 0x48, 0x33, 0xc4, 0x48, 0x89, 0x85, 0xf0, 0x00, 0x00, 0x00,
  0x4d, 0x8b, 0xf0, 0x4c, 0x8b, 0xfa, 0x48, 0x8b, 0xf9, 0x48, 0x89, 0x54, 0x24, 0x48, 0x4c, 0x89,
};
static uint8_t *appVerifyProductKeyFSig = appVerifyProductKeyFSig_;
static size_t appVerifyProductKeyFSigSize = sizeof(appVerifyProductKeyFSig_);

static uint8_t setOkFlagFSig_[] = {
  0xcc, 0xc6, 255 , 255 , 255 , 255 , 255 , 0x01, 0xc3,
};
static uint8_t *setOkFlagFSig = setOkFlagFSig_;
static size_t setOkFlagFSigSize = sizeof(setOkFlagFSig_);

static bool blockInternet = true;

static bool debugMode = false;

static char *consoleFile = NULL;

static const char *synthVSupportedVersions =
  "Pro 1.11.0b1\n"
  "Pro 1.11.0b2\n"
  "Pro 1.11.0\n"
  "Pro 1.11.1\n"
  "Pro 1.11.2\n"
  "";

static bool checkSynthVVersion = true;

static bool inDllMain = false;
static DWORD injectTime = 0;

static bool hasAllocatedConsole = false;
static const char *synthVDetected;

void allocConsoleOnce() {
  if (!hasAllocatedConsole) {
    if (!consoleFile) {
      AllocConsole();
      freopen("CONOUT$", "wt", stdout);
    } else {
      freopen(consoleFile, "at", stdout);
      puts("=== NEW SESSION ===");
    }

    hasAllocatedConsole = true;
  }
}

void dumpStack() {
  allocConsoleOnce();
  puts("*** STACK TRACE ***");

  HANDLE base = GetModuleHandleW(NULL);
  LPVOID stack[16];

  uint16_t frames = RtlCaptureStackBackTrace(1, 16, stack, NULL);
  for (int i = 0; i < frames; i++) {
    LPVOID func = stack[i];
    printf("- [%d] %p (%p)\n", i, func, (uintptr_t)func > (uintptr_t)base ? func - base : 0xffffffff);
  }
}

void _Noreturn _die(int line) {
  if (consoleFile) {
    consoleFile = NULL;
    hasAllocatedConsole = false;
  }

  allocConsoleOnce();

  printf("YumeKey Obsidian has encountered a fatal error.\n\n");
  printf("Please include the below information in your report:\n");
  printf("@ lifeCycle: %s\n", _lifeCycle);
  printf("+ line: %d\n", line);
  printf("inDllMain = %s\n", inDllMain ? "true" : "false");
  printf("timeSinceInject = %d\n", GetTickCount() - injectTime);
  printf("Obsidian version: %d.%d.%d in Synthesizer V Studio %s\n",
         VER_MAJOR, VER_MINOR, VER_PATCH, synthVDetected ? synthVDetected : "(unknown)");
  system("ver");
  printf("\n");
  dumpStack();

  printf("\nPress any key to exit...\n");

  _getch();
  ExitProcess(42);
}

static uint8_t hatoi8(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  die();
}

size_t parseHex(uint8_t *dst, const char *src) {
  size_t i = 0;
  while (*src) {
    *dst++ = hatoi8(*(src + 1)) | (hatoi8(*src) << 4);
    src += 2;
    i++;
  }
  return i;
}

int memCmpWithMask(const void *a, const void *b, size_t n) {
  const unsigned char *x = a;
  const unsigned char *y = b;

  for (size_t i = 0; i < n; i++) {
    if (x[i] != y[i] && x[i] != 255) {
      return x[i] - y[i];
    }
  }
  return 0;
}

void *memMem(const void *haystack, size_t n, const void *needle, size_t m) {
  if (m > n || !m || !n)
    return NULL;
  if (__builtin_expect((m > 1), 1)) {
    const unsigned char *y = haystack;
    const unsigned char *x = needle;
    size_t j = 0;
    size_t k = 1, l = 2;
    if (x[0] == x[1]) {
      k = 2;
      l = 1;
    }
    while (j <= n-m) {
      if (x[1] != y[j+1]) {
        j += k;
      } else {
        if (!memCmpWithMask(x+2, y+j+2, m-2) && (x[0] == y[j] || x[0] == 255))
          return (void*) &y[j];
        j += l;
      }
    }
  } else {
    /* degenerate case */
    return memchr(haystack, ((unsigned char*)needle)[0], n);
  }
  return NULL;
}

LPVOID safeMemMem(HANDLE hProcess, LPVOID pvHaystack, SIZE_T szHaystackSize, LPVOID pvNeedle, SIZE_T szNeedleSize) {
  uint8_t buffer[4096];

  for (SIZE_T i = 0; i < szHaystackSize; i += sizeof(buffer)) {
    SIZE_T bytesRead;

    if (!ReadProcessMemory(hProcess, (LPBYTE)pvHaystack + i, buffer, sizeof(buffer), &bytesRead) || bytesRead == 0) {
      continue;
    }

    LPVOID found = memMem(buffer, bytesRead, pvNeedle, szNeedleSize);
    if (found != NULL) {
      return (LPBYTE)pvHaystack + i + ((LPBYTE)found - buffer);
    }
  }

  return NULL;
}

static void (*orig_getActivationStatus)(struct ProductBase *thisPtr, struct FunctionBase *success, struct FunctionBase *failure);

void hook_getActivationStatus(struct ProductBase *thisPtr, struct FunctionBase *success, struct FunctionBase *failure) {
  thisPtr->isActivated = true;
  thisPtr->isFree = true;
  thisPtr->hasNoProductKey = true;
  orig_getActivationStatus(thisPtr, success, success);
}

static void (*orig_voiceDBVerifyProductKey)(struct ProductBase *thisPtr, struct FunctionBase *success, struct FunctionBase *failure);

static void hook_voiceDBVerifyProductKey(struct ProductBase *thisPtr, struct FunctionBase *success, struct FunctionBase *failure) {
  thisPtr->isActivated = true;
  thisPtr->isFree = true;
  thisPtr->hasNoProductKey = true;
  orig_voiceDBVerifyProductKey(thisPtr, success, success);
}

static void (*orig_appVerifyProductKey)(struct ProductBase *thisPtr, struct FunctionBase *success, struct FunctionBase *failure);

static void hook_appVerifyProductKey(struct ProductBase *thisPtr, struct FunctionBase *success, struct FunctionBase *failure) {
  thisPtr->isActivated = true;
  thisPtr->isFree = true;
  thisPtr->hasNoProductKey = true;
  printf("Product name: %s, Product vendor: %s\n", thisPtr->name, thisPtr->vendor);
  orig_appVerifyProductKey(thisPtr, success, success);
}

static WINAPI BOOL (*orig_GetVolumeInformationW)(
       LPCWSTR, LPWSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPWSTR, DWORD);

static WINAPI BOOL hook_GetVolumeInformationW(LPCWSTR lpRootPathName,
       LPWSTR lpVolumeNameBuffer, DWORD nVolumeNameSize,
       LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength,
       LPDWORD lpFileSystemFlags, LPWSTR lpFileSystemNameBuffer,
       DWORD nFileSystemNameSize) {

  static bool secondaryHooksReady = false;

  if (!secondaryHooksReady) {
    if (checkSynthVVersion) {
      LIFECYCLE("CHECK_VERSION");
      printf("[Obsidian] Running compatibility check...\n");

      const char *svVer = NULL;
      for (int i = 0; i < 3; i++) {
        svVer = safeMemMem(GetCurrentProcess(), GetProcessHeap(), 64 * 1024 * 1024, "Pro 1.", sizeof("Pro 1.") - 1);

        if (!svVer) {
          if (safeMemMem(GetCurrentProcess(), GetProcessHeap(), 64 * 1024 * 1024, "Basic 1.", sizeof("Basic 1.") - 1)) {
            allocConsoleOnce();
            printf("[Obsidian] Synthesizer V Studio Basic is not supported. This program will now exit.\n\n");
            die();
          }
        } else {
          break;
        }

        Sleep(500);
      }

      if (!svVer) {
        allocConsoleOnce();
        printf("Failed to detect Synthesizer V Studio version.\n\n");
        die();
      }

      printf("[Obsidian] Loaded into Synthesizer V Studio %s\n", svVer);
      synthVDetected = svVer;

      if (!strstr(synthVSupportedVersions, svVer)) {
        if (consoleFile) {
          consoleFile = NULL;
          hasAllocatedConsole = false;
        }

        allocConsoleOnce();
        printf("[Obsidian] Synthesizer V Studio %s is NOT SUPPORTED. If you continue, there may be problems!\n\n");
        printf("\n\n");
        printf("Press any key to continue anyway...\n");
        _getch();
      }
    }

    LIFECYCLE("INIT_HOOKS_2");
    printf("[Obsidian] Initializing secondary hooks...\n");

    if (fptrAddBase == 1) {
      fptrAddBase = (uintptr_t)GetModuleHandleW(NULL);
    }

    LPVOID exeBase = GetModuleHandleW(NULL);
    SIZE_T exeSize = 64 * 1024 * 1024;

    if (getActivationStatusFPtr != NULL) {
      debug_printf("[Obsidian] Using getActivationStatus function offset %p + %p.\n", getActivationStatusFPtr, fptrAddBase);
      getActivationStatusFPtr += fptrAddBase;
    } else {
      debug_printf("[Obsidian] Attempting to autodetect getActivationStatus function...\n");
      getActivationStatusFPtr = safeMemMem(GetCurrentProcess(), exeBase, exeSize, getActivationStatusFSig, getActivationStatusFSigSize);
      if (!getActivationStatusFPtr) {
        allocConsoleOnce();
        debug_printf("Failed to autodetect and initialize required getActivationStatus patch.\n\n");
        die();
      }
    }

    debug_printf("[Obsidian] getActivationStatus function @ %p\n", getActivationStatusFPtr);

    if (voiceDBVerifyProductKeyFPtr != NULL) {
      debug_printf("[Obsidian] Using voiceDBVerifyProductKey function offset %p + %p.\n", voiceDBVerifyProductKeyFPtr, fptrAddBase);
      voiceDBVerifyProductKeyFPtr += fptrAddBase;
    } else {
      debug_printf("[Obsidian] Attempting to autodetect voiceDBVerifyProductKey function...\n");
      voiceDBVerifyProductKeyFPtr = safeMemMem(GetCurrentProcess(), exeBase, exeSize, voiceDBVerifyProductKeyFSig, voiceDBVerifyProductKeyFSigSize);
      if (!voiceDBVerifyProductKeyFPtr) {
        allocConsoleOnce();
        debug_printf("Failed to autodetect and initialize required voiceDBVerifyProductKey patch.\n\n");
        die();
      }
    }

    debug_printf("[Obsidian] voiceDBVerifyProductKey function @ %p\n", voiceDBVerifyProductKeyFPtr);

    if (appVerifyProductKeyFPtr != NULL) {
      debug_printf("[Obsidian] Using appVerifyProductKey function offset %p + %p.\n", appVerifyProductKeyFPtr, fptrAddBase);
      appVerifyProductKeyFPtr += fptrAddBase;
    } else {
      debug_printf("[Obsidian] Attempting to autodetect appVerifyProductKey function...\n");

      LPVOID searchBase = exeBase;
      if (appVerifyProductKeyFSigSize <= voiceDBVerifyProductKeyFSigSize &&
          !memCmpWithMask(voiceDBVerifyProductKeyFSig, appVerifyProductKeyFSig, appVerifyProductKeyFSigSize)) {
        searchBase = voiceDBVerifyProductKeyFPtr + voiceDBVerifyProductKeyFSigSize;
        debug_printf("[Obsidian] Starting search at %p.\n", searchBase);
      }

      appVerifyProductKeyFPtr = safeMemMem(GetCurrentProcess(), searchBase, exeSize, appVerifyProductKeyFSig, appVerifyProductKeyFSigSize);
      if (!appVerifyProductKeyFPtr) {
        allocConsoleOnce();
        debug_printf("Failed to autodetect and initialize required appVerifyProductKey patch.\n\n");
        die();
      }
    }

    debug_printf("[Obsidian] appVerifyProductKey function @ %p\n", appVerifyProductKeyFPtr);

    if (setOkFlagFPtr != NULL) {
      debug_printf("[Obsidian] Using setOkFlag function offset %p + %p.\n", setOkFlagFPtr, fptrAddBase);
      setOkFlagFPtr += fptrAddBase;
    } else {
      debug_printf("[Obsidian] Attempting to autodetect setOkFlag function...\n");
      setOkFlagFPtr = safeMemMem(GetCurrentProcess(), exeBase, exeSize, setOkFlagFSig, setOkFlagFSigSize);
      if (!setOkFlagFPtr) {
        allocConsoleOnce();
        debug_printf("Failed to autodetect and call setOkFlag.\n\n");
        die();
      }
      while (*(uint8_t*)setOkFlagFPtr == 0xcc) setOkFlagFPtr++;
    }

    debug_printf("[Obsidian] setOkFlag function @ %p\n", setOkFlagFPtr);
    void (*setOkFlag)() = setOkFlagFPtr;
    setOkFlag();

    if (MH_CreateHook(getActivationStatusFPtr, hook_getActivationStatus, (LPVOID*)&orig_getActivationStatus) != MH_OK ||
        MH_EnableHook(getActivationStatusFPtr)) {
      die();
    }

    if (MH_CreateHook(voiceDBVerifyProductKeyFPtr, hook_voiceDBVerifyProductKey, (LPVOID*)&orig_voiceDBVerifyProductKey) != MH_OK ||
        MH_EnableHook(voiceDBVerifyProductKeyFPtr)) {
      die();
    }

    if (MH_CreateHook(appVerifyProductKeyFPtr, hook_appVerifyProductKey, (LPVOID*)&orig_appVerifyProductKey) != MH_OK ||
        MH_EnableHook(appVerifyProductKeyFPtr)) {
      die();
    }

    secondaryHooksReady = true;
    printf("[Obsidian] Secondary hooks ready.\n");
    LIFECYCLE("RUNNING");
  }

  return orig_GetVolumeInformationW(lpRootPathName,
         lpVolumeNameBuffer, nVolumeNameSize,
         lpVolumeSerialNumber, lpMaximumComponentLength,
         lpFileSystemFlags, lpFileSystemNameBuffer,
         nFileSystemNameSize);
}

static bool initConfig() {
  WCHAR configPath[MAX_PATH];
  GetModuleFileNameW(GetModuleHandleW(0), configPath, sizeof(configPath));
  WCHAR *baseName = wcsrchr(configPath, '\\');
  wcscpy(baseName + 1, L"obsidian.ini");

  printf("[Obsidian] Trying config file: ");
  _putws(configPath);

  FILE *fp = _wfopen(configPath, L"r");
  if (!fp) {
    printf("[Obsidian] No config file.\n");
    goto skip;
  }

  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) {
    int len = strlen(line);

    char *p = &line[len - 1];
    while (p >= line && strchr("\r\n\t\v ", *p)) {
      *p = '\0';
      p--;
    }

    char *l = line;
    while (*l != '\0' && strchr("\r\n\t\v ", *l)) {
      l++;
    }

    if (*l == '\0' || *l == '#' || *l == ';') {
      continue;
    }

    len = strlen(l);

    printf("[Obsidian] Parsing config line: %s\n", l);

    char *nBuf = malloc(len);
    strcpy(nBuf, l);

    char *eq = strchr(l, '=');
    if (!eq) {
      printf("[Obsidian] Invalid configuration line: %s\n", l);
      free(nBuf);
      continue;
    }

    *eq = '\0';
    if (getenv(l)) {
      printf("[Obsidian] Skipping: not overriding environment variable.\n");
      free(nBuf);
      continue;
    }

    putenv(nBuf);
  }

skip:
  printf("[Obsidian] Initializing configuration from environment...\n");

  if (getenv("OBSIDIAN_BASE_ADDR")) {
    fptrAddBase = strtoull(getenv("OBSIDIAN_BASE_ADDR"), NULL, 0);
  }

#ifdef DEBUG
  if (getenv("OBSIDIAN_GETACTIVATIONSTATUS_ADDR")) {
    getActivationStatusFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_GETACTIVATIONSTATUS_ADDR"), NULL, 0);
  }

  if (getenv("OBSIDIAN_VOICEDB_VERIFYPRODUCTKEY_ADDR")) {
    voiceDBVerifyProductKeyFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_VOICEDB_VERIFYPRODUCTKEY_ADDR"), NULL, 0);
  }

  if (getenv("OBSIDIAN_APP_VERIFYPRODUCTKEY_ADDR")) {
    appVerifyProductKeyFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_APP_VERIFYPRODUCTKEY_ADDR"), NULL, 0);
  }

  if (getenv("OBSIDIAN_SETOKFLAG_ADDR")) {
    setOkFlagFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_SETOKFLAG_ADDR"), NULL, 0);
  }
#else
  if (getenv("OBSIDIAN_GAFKALO")) {
    getActivationStatusFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_GAFKALO"), NULL, 0);
  }

  if (getenv("OBSIDIAN_VODPKO")) {
    voiceDBVerifyProductKeyFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_VODPKO"), NULL, 0);
  }

  if (getenv("OBSIDIAN_AOVEKAO")) {
    appVerifyProductKeyFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_AOVEKAO"), NULL, 0);
  }

  if (getenv("OBSIDIAN_SAOSKFP")) {
    setOkFlagFPtr = (LPVOID)strtoull(getenv("OBSIDIAN_SAOSKFP"), NULL, 0);
  }
#endif

  if (getenv("OBSIDIAN_BLOCK_INTERNET")) {
    blockInternet = strtoull(getenv("OBSIDIAN_BLOCK_INTERNET"), NULL, 0) != 0;
  }

  if (getenv("OBSIDIAN_DEBUG_MODE")) {
    debugMode = atoi(getenv("OBSIDIAN_DEBUG_MODE"));
  }

  if (getenv("OBSIDIAN_CONSOLE_FILE")) {
    consoleFile = getenv("OBSIDIAN_CONSOLE_FILE");
  }

  if (getenv("OBSIDIAN_CHECK_SYNTHV_VERSION")) {
    checkSynthVVersion = strtoull(getenv("OBSIDIAN_CHECK_SYNTHV_VERSION"), NULL, 0) != 0;
  }

  if (debugMode) {
    allocConsoleOnce();
  }

  printf("[Obsidian] Configuration loaded.\n");

  return true;
}

static void *orig_InternetOpenW;

static void *hook_InternetOpenW() {
  return NULL;
}

static bool initHooks() {
#define MUST(x) if ((x) != MH_OK) return false;
  MUST(MH_Initialize());

  MUST(MH_CreateHookApi(L"kernel32", "GetVolumeInformationW",
                        &hook_GetVolumeInformationW,
                        (LPVOID*)&orig_GetVolumeInformationW));

  if (blockInternet) {
    MUST(MH_CreateHookApi(L"wininet", "InternetOpenW",
                          &hook_InternetOpenW,
                          (LPVOID*)&orig_InternetOpenW));
  }

  MUST(MH_EnableHook(MH_ALL_HOOKS));
#undef MUST
  return true;
}

__declspec(dllexport) BOOL WINAPI DllMain(
            HINSTANCE thisDll, DWORD reason, LPVOID reserved) {
  (void)reserved;

  switch (reason) {
    case DLL_PROCESS_ATTACH:
      if (FindAtomA("Obsidian")) {
        break;
      }

      AddAtomA("Obsidian");

      if (getenv("OBSIDIAN_DEBUG_MODE") && getenv("OBSIDIAN_DEBUG_MODE")[0] > '0') {
        debugMode = 1;
      }

      bool earlyDebug = debugMode > 0;

      if (debugMode) {
        allocConsoleOnce();
        printf("(%p) YumeKey Obsidian %d.%d.%d initializing...\n", thisDll, VER_MAJOR, VER_MINOR, VER_PATCH);
        printf("Copyright (C) 2023-2024 YumeCorp International. All rights reserved.\n");
        printf("*** IF YOU PAID FOR THIS SOFTWARE, YOU'VE BEEN SCAMMED! ***\n");

        WCHAR dllPathW[MAX_PATH];
        GetModuleFileNameW(thisDll, dllPathW, sizeof(dllPathW) / sizeof(WCHAR));
        printf("[Obsidian] Path: ");
        _putws(dllPathW);
      }

      inDllMain = true;
      injectTime = GetTickCount();

      LIFECYCLE("INIT_CONFIG");
      if (!initConfig()) die();

      LIFECYCLE("INIT_HOOKS");
      if (!initHooks()) die();

      LIFECYCLE("PIN");
      WCHAR dllPathW[MAX_PATH];
      GetModuleFileNameW(thisDll, dllPathW, sizeof(dllPathW) / sizeof(WCHAR));
      LoadLibraryW(dllPathW);

      LIFECYCLE("PRE_INIT_HOOKS_2");
      if (debugMode > 0 && !earlyDebug) {
        printf("[Obsidian] Some debug messages from before configuration was loaded were hidden. "
               "Set the environment variable OBSIDIAN_DEBUG_MODE before launching in order to see them.");
        printf("[Obsidian] Current version: %d.%d.%d\n", VER_MAJOR, VER_MINOR, VER_PATCH);
      }

#ifndef DEBUG
      if (debugMode) {
        printf("[Obsidian] Debug mode: not supported in release builds.\n");
      }
#endif

      printf("[Obsidian] Ready.\n");
      inDllMain = false;

      break;
    case DLL_PROCESS_DETACH:
      MH_DisableHook(MH_ALL_HOOKS);
      MH_Uninitialize();

      printf("[Obsidian] Detaching...\n");
      DeleteAtom(FindAtomA("Obsidian"));
  }

  return TRUE;
}
