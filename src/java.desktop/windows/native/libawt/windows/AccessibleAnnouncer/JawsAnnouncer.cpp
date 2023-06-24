/*
 * Copyright (c) 2023, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2023, JetBrains s.r.o.. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include "JawsAnnouncer.h"

#ifndef NO_A11Y_JAWS_ANNOUNCING
#include "IJawsApi.h"
#include "sun_swing_AccessibleAnnouncer.h"
#include "jni_util.h"                           // JNU_ThrowOutOfMemoryError
#include "debug_assert.h"                       // DASSERT
#include <windows.h>                            // GetCurrentThreadId
#include <initguid.h>                           // DEFINE_GUID
#include <awt.h>


namespace
{
    class Idea321176Logger final
    {
    public: // forbids copies and moves
        Idea321176Logger(const Idea321176Logger&) = delete;
        Idea321176Logger(Idea321176Logger&&) = delete;

        Idea321176Logger& operator=(const Idea321176Logger&) = delete;
        Idea321176Logger& operator=(Idea321176Logger&&) = delete;

    public:
        /**
         * Log entry:
         *   [<threadId>] [<date-time>] args...<newline>
         *   <newline>
         */
        template<typename T1, typename... Ts>
        static void logEntry(const T1& val1, const Ts&... values)
        {
            TLSBuffer::reset();

            const auto threadId = ::GetCurrentThreadId();
            const auto dateTime = [] {
                SYSTEMTIME time{};
                ::GetSystemTime(&time);
                return time;
            }();

            TLSBuffer::appendFormatted('[', threadId, "] [", dateTime, "] ", val1, values..., '\n');
            TLSBuffer::appendFormatted('\n');

            const auto payload = TLSBuffer::getPayload();
            if ((payload.payloadUtf8 == nullptr) || (payload.payloadLength < 1))
            {
                constexpr char errStr[] = "Idea321176Logger::logEntry: FAILED TO CONSTRUCT A STRING TO LOG";
                constexpr auto errStrLen = sizeof(errStr) / sizeof(errStr[0]) - 1;

                javaSystemErrPrint(errStr, errStrLen);

                return;
            }

            javaSystemErrPrint(payload.payloadUtf8, payload.payloadLength);
        }

    private: // ctors/dtor
        Idea321176Logger() = default;
        ~Idea321176Logger() = default;

    private:
        class TLSBuffer final
        {
        public:
            struct View final
            {
                const char* const payloadUtf8;
                const size_t payloadLength;
            };

        public:
            static View getPayload()
            {
                auto& instance = getInstance();

                if ((instance.buffer_ == nullptr) || (instance.length_ < 1))
                {
                    return { nullptr, 0 };
                }

                return { instance.buffer_, instance.length_ };
            }

        public:
            static void reset()
            {
                getInstance().resetImpl();
            }

        public:
            template<typename T1, typename T2, typename... Ts>
            static void appendFormatted(const T1& val1, const T2& val2, const Ts&... values)
            {
                appendFormatted(val1);
                appendFormatted(val2, values...);
            }

            static void appendFormatted(const bool value) { appendFormatted(value ? "true" : "false"); }

            static void appendFormatted(const char value)
            {
                const int intVal = value;
                getInstance().appendFormattedImpl("%c", intVal);
            }

            static void appendFormatted(const signed char value) { appendFormatted(static_cast<signed long long>(value)); }

            static void appendFormatted(const unsigned char value) { appendFormatted(static_cast<unsigned long long>(value)); }

            /*void Jbs9571512976146Logger::TLSBuffer::appendFormatted(const wchar_t value)
            {
                const int intVal = value;
                getInstance().appendFormattedImpl(L"%c", intVal, intVal);
            }*/

            static void appendFormatted(const signed short value) { appendFormatted(static_cast<signed long long>(value)); }

            static void appendFormatted(const unsigned short value) { appendFormatted(static_cast<unsigned long long>(value)); }

            static void appendFormatted(const signed int value) { appendFormatted(static_cast<signed long long>(value)); }
            static void appendFormatted(const unsigned int value) { appendFormatted(static_cast<unsigned long long>(value)); }

            static void appendFormatted(const signed long value) { appendFormatted(static_cast<signed long long>(value)); }
            static void appendFormatted(const unsigned long value) { appendFormatted(static_cast<unsigned long long>(value)); }

            static void appendFormatted(const signed long long value) { getInstance().appendFormattedImpl("%lld", value); }
            static void appendFormatted(const unsigned long long value) { getInstance().appendFormattedImpl("%llu", value); }

            static void appendFormatted(const float value) { appendFormatted(static_cast<double>(value)); }
            static void appendFormatted(const double value) { getInstance().appendFormattedImpl("%f", value); }
            static void appendFormatted(const long double value) { getInstance().appendFormattedImpl("%Lf", value); }

            static void appendFormatted(std::nullptr_t) { getInstance().appendFormattedImpl("%s", "nullptr"); }

            static void appendFormatted(const char* const str)
            {
                if (str == nullptr) {
                    getInstance().appendFormattedImpl("%s", "(char*)nullptr");
                } else {
                    getInstance().appendFormattedImpl("%s", str);
                }
            }

            static void appendFormatted(const wchar_t* const str)
            {
                if (str == nullptr) {
                    getInstance().appendFormattedImpl("%s", "(wchar_t*)nullptr");
                } else {
                    getInstance().appendFormattedImpl("%ls", str);
                }
            }

            static void appendFormatted(const void* const ptr)
            {
                if (ptr == nullptr) {
                    getInstance().appendFormattedImpl("%s", "nullptr");
                } else {
                    getInstance().appendFormattedImpl("0x%p", ptr);
                }
            }

            static void appendFormatted(const ::SYSTEMTIME& dateTime)
            {
                getInstance().appendFormattedImpl(
                    "%02u.%02u.%u %02u:%02u:%02u.%03u",
                    static_cast<unsigned>(dateTime.wDay),
                    static_cast<unsigned>(dateTime.wMonth),
                    static_cast<unsigned>(dateTime.wYear),
                    static_cast<unsigned>(dateTime.wHour),
                    static_cast<unsigned>(dateTime.wMinute),
                    static_cast<unsigned>(dateTime.wSecond),
                    static_cast<unsigned>(dateTime.wMilliseconds)
                );
            }

        private:
            char* buffer_;
            size_t length_;
            size_t charsCapacity_;

        private:
            static TLSBuffer& getInstance()
            {
                static thread_local TLSBuffer result;
                return result;
            }

        private:
            explicit TLSBuffer()
                : buffer_(nullptr)
                , charsCapacity_(0)
                , length_(0)
            {}

            ~TLSBuffer() { freeImpl(); }

        private:
            void reallocImpl(const size_t newCharsCapacity)
            {
                void* newBuffer = nullptr;

                try
                {
                    newBuffer = ::safe_Realloc(buffer_, newCharsCapacity * sizeof(*buffer_));
                }
                catch(...)
                {
                }

                if (newBuffer == nullptr)
                {
                    freeImpl();
                    return;
                }

                buffer_ = reinterpret_cast<decltype(buffer_)>(newBuffer);
                charsCapacity_ = newCharsCapacity;

                if (charsCapacity_ < 1)
                {
                    charsCapacity_ = length_ = 0;
                }

                if (length_ > charsCapacity_)
                {
                    length_ = charsCapacity_ - 1;
                    buffer_[length_] = 0;
                }
            }

            void freeImpl()
            {
                if (buffer_ != nullptr)
                {
                    ::free(buffer_);
                }

                buffer_ = nullptr;
                charsCapacity_ = 0;
                length_ = 0;
            }

            void resetImpl()
            {
                length_ = 0;
                if (charsCapacity_ > 0)
                {
                    buffer_[0] = 0;
                }
            }

        private:
            template<typename... Ts>
            void appendFormattedImpl(const char* const formatStr, const Ts&... params)
            {
                const auto getFreeSpace = [this] { return (charsCapacity_ > length_) ? charsCapacity_ - length_ - 1 : 0; };

                int written = -1;
                if ((charsCapacity_ > 0) && (charsCapacity_ > length_))
                {
                    written = ::snprintf(buffer_ + length_, getFreeSpace(), formatStr, params...);
                }

                while ((written < 0) || (written >= getFreeSpace()))
                {
                    reallocImpl( (charsCapacity_ < 1) ? 1024 : (charsCapacity_ * 2) );
                    written = ::snprintf(buffer_ + length_, getFreeSpace(), formatStr, params...);
                }

                length_ += written;
                buffer_[length_] = 0;
            }
        };

    private:
        // System.err.print(...)
        static void javaSystemErrPrint(const char* utf8Str, size_t charsLen)
        {
            if (jvm == nullptr) {
                return;
            }

            JNIEnv* const env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
            if (env == nullptr) {
                return;
            }

            jboolean exceptionOccurred = JNI_FALSE;
            jvalue systemErr = JNU_GetStaticFieldByName(env, &exceptionOccurred, "java/lang/System", "err", "Ljava/io/PrintStream;");
            if (exceptionOccurred == JNI_TRUE) {
                return;
            }

            //static_assert(sizeof(*utf16Str) == sizeof(jchar), "Cannot cast utf16Str to jchar*");

            jstring strToPrint = env->NewStringUTF(/*reinterpret_cast<const jchar*>(*/utf8Str/*)*/);
            if (strToPrint == nullptr) {
                return;
            }

            (void)JNU_CallMethodByName(env, &exceptionOccurred, systemErr.l, "print", "(Ljava/lang/String;)V", strToPrint);

            env->DeleteLocalRef(strToPrint);
            strToPrint = nullptr;
        }
    };
}


/* {CCE5B1E5-B2ED-45D5-B09F-8EC54B75ABF4} */
DEFINE_GUID(CLSID_JAWSCLASS,
    0xCCE5B1E5, 0xB2ED, 0x45D5, 0xB0, 0x9F, 0x8E, 0xC5, 0x4B, 0x75, 0xAB, 0xF4);

/* {123DEDB4-2CF6-429C-A2AB-CC809E5516CE} */
DEFINE_GUID(IID_IJAWSAPI,
    0x123DEDB4, 0x2CF6, 0x429C, 0xA2, 0xAB, 0xCC, 0x80, 0x9E, 0x55, 0x16, 0xCE);


class ComInitializationWrapper final {
public: // ctors
    ComInitializationWrapper() = default;
    ComInitializationWrapper(const ComInitializationWrapper&) = delete;
    ComInitializationWrapper(ComInitializationWrapper&&) = delete;

public: // assignments
    ComInitializationWrapper& operator=(const ComInitializationWrapper&) = delete;
    ComInitializationWrapper& operator=(ComInitializationWrapper&&) = delete;

public:
    HRESULT tryInitialize() {
        if (!isInitialized()) {
            m_initializeResult = CoInitialize(nullptr);
        }
        return m_initializeResult;
    }

public: // dtor
    ~ComInitializationWrapper() {
        // MSDN: To close the COM library gracefully, each successful call to CoInitialize or CoInitializeEx,
        //       including those that return S_FALSE, must be balanced by a corresponding call to CoUninitialize
        if ((m_initializeResult == S_OK) || (m_initializeResult == S_FALSE)) {
            m_initializeResult = CO_E_NOTINITIALIZED;
            CoUninitialize();
        }
    }

public: // getters
    HRESULT getInitializeResult() const noexcept { return m_initializeResult; }

    bool isInitialized() const noexcept {
        if ( (m_initializeResult == S_OK) ||
             (m_initializeResult == S_FALSE) ||             // Is already initialized
             (m_initializeResult == RPC_E_CHANGED_MODE) ) { // Is already initialized but with different threading mode
            return true;
        }
        return false;
    }

private:
    HRESULT m_initializeResult = CO_E_NOTINITIALIZED;
};


template<typename T>
struct ComObjectWrapper final {
    T* objPtr;

    ~ComObjectWrapper() {
        T* const localObjPtr = objPtr;
        objPtr = nullptr;

        if (localObjPtr != nullptr) {
            localObjPtr->Release();
        }
    }
};


bool JawsAnnounce(JNIEnv *env, jstring str, jint priority)
{
    Idea321176Logger::logEntry("JawsAnnounce(env=", env, ", str=", str, "priority=", priority, ")");

    DASSERT(env != nullptr);
    DASSERT(str != nullptr);

    static const DWORD comInitThreadId = ::GetCurrentThreadId();

    const DWORD currThread = ::GetCurrentThreadId();
    if (currThread != comInitThreadId) {
#ifdef DEBUG
        fprintf(stderr, "JawsAnnounce: currThread != comInitThreadId.\n");
#endif
        Idea321176Logger::logEntry("<- JawsAnnounce: currThread(", currThread, ") != comInitThreadId(", comInitThreadId, ")");
        return false;
    }

    Idea321176Logger::logEntry("JawsAnnounce: trying to initialize COM...");

    static ComInitializationWrapper comInitializer;
    comInitializer.tryInitialize();
    if (!comInitializer.isInitialized()) {
#ifdef DEBUG
        fprintf(stderr, "JawsAnnounce: CoInitialize failed ; HRESULT=0x%llX.\n",
                static_cast<unsigned long long>(comInitializer.getInitializeResult()));
#endif
        Idea321176Logger::logEntry("<- JawsAnnounce: CoInitialize failed ; HRESULT=", comInitializer.getInitializeResult());
        return false;
    }

    Idea321176Logger::logEntry("JawsAnnounce: COM is initialized.");

    Idea321176Logger::logEntry("JawsAnnounce: trying to initialize pJawsApi instance...");

    static ComObjectWrapper<IJawsApi> pJawsApi{ nullptr };
    if (pJawsApi.objPtr == nullptr) {
        HRESULT hr = CoCreateInstance(CLSID_JAWSCLASS, nullptr, CLSCTX_INPROC_SERVER, IID_IJAWSAPI, reinterpret_cast<void**>(&pJawsApi.objPtr));
        if ((hr != S_OK) || (pJawsApi.objPtr == nullptr)) {
#ifdef DEBUG
            fprintf(stderr, "JawsAnnounce: CoCreateInstance failed ; HRESULT=0x%llX.\n", static_cast<unsigned long long>(hr));
#endif
            // just in case
            if (pJawsApi.objPtr != nullptr) {
                pJawsApi.objPtr->Release();
                pJawsApi.objPtr = nullptr;
            }

            Idea321176Logger::logEntry("<- JawsAnnounce: CoCreateInstance failed ; HRESULT=", hr);

            return false;
        }
    }

    Idea321176Logger::logEntry("JawsAnnounce: pJawsApi is initialized.");

    Idea321176Logger::logEntry("JawsAnnounce: obtaining the string to speak...");

    VARIANT_BOOL jawsInterruptCurrentOutput = VARIANT_TRUE;
    if (priority == sun_swing_AccessibleAnnouncer_ANNOUNCE_WITHOUT_INTERRUPTING_CURRENT_OUTPUT) {
        jawsInterruptCurrentOutput = VARIANT_FALSE;
    }

    const jchar* jStringToSpeak = env->GetStringChars(str, nullptr);
    if (jStringToSpeak == nullptr) {
        if (env->ExceptionCheck() == JNI_FALSE) {
            JNU_ThrowOutOfMemoryError(env, "JawsAnnounce: failed to obtain chars from the announcing string");
        }

        Idea321176Logger::logEntry("<- JawsAnnounce: jStringToSpeak=nullptr");

        return false;
    }

    Idea321176Logger::logEntry("JawsAnnounce: jStringToSpeak=\"", reinterpret_cast<const wchar_t*>(jStringToSpeak), "\"");

    BSTR stringToSpeak = SysAllocString(jStringToSpeak);

    Idea321176Logger::logEntry("JawsAnnounce: stringToSpeak=", stringToSpeak);

    env->ReleaseStringChars(str, jStringToSpeak);
    jStringToSpeak = nullptr;

    if (stringToSpeak == nullptr) {
        if (env->ExceptionCheck() == JNI_FALSE) {
            JNU_ThrowOutOfMemoryError(env, "JawsAnnounce: failed to allocate memory for the announcing string");
        }

        Idea321176Logger::logEntry("<- JawsAnnounce: stringToSpeak=nullptr");

        return false;
    }

    VARIANT_BOOL jawsSucceeded = VARIANT_FALSE;

    Idea321176Logger::logEntry("JawsAnnounce: trying to say the string through COM...");

    HRESULT comCallResult = pJawsApi.objPtr->SayString(stringToSpeak, jawsInterruptCurrentOutput, &jawsSucceeded);

    Idea321176Logger::logEntry("JawsAnnounce: the COM call has finished...");

    SysFreeString(stringToSpeak);
    stringToSpeak = nullptr;

    Idea321176Logger::logEntry("JawsAnnounce: stringToSpeak has been freed");

    if (FAILED(comCallResult)) {
#ifdef DEBUG
        fprintf(stderr, "JawsAnnounce: failed to invoke COM function to say string ; HRESULT=0x%llX.\n", static_cast<unsigned long long>(comCallResult));
#endif
        Idea321176Logger::logEntry("<- JawsAnnounce: the COM call has failed ; HRESULT=", comCallResult);
        return false;
    }
    if (jawsSucceeded != VARIANT_TRUE) {
#ifdef DEBUG
        fprintf(stderr, "JawsAnnounce: failed to say string ; code = %d.\n", static_cast<int>(jawsSucceeded));
#endif
        Idea321176Logger::logEntry("<- JawsAnnounce: failed to announce the string ; code=", jawsSucceeded);
        return false;
    }

    Idea321176Logger::logEntry("<- JawsAnnounce: SUCCEEDED.");

    return true;
}

#endif // ndef NO_A11Y_JAWS_ANNOUNCING
