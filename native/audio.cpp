// audio capture from microphone

#include <node.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <iostream>
#include <vector>
#include <thread>

using namespace v8;

// Global variables for audio capture
IMMDeviceEnumerator *enumerator = nullptr;
IMMDevice *device = nullptr;
IAudioClient *audioClient = nullptr;
IAudioCaptureClient *captureClient = nullptr;
std::vector<BYTE> audioData;
bool capturing = false;

// Function declarations
void StartCapture();
void StopCapture();

// Function to start audio capture
void StartCapture()
{
    CoInitialize(nullptr); // Initialize COM library
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void **)&enumerator);

    if (FAILED(hr))
    {
        std::cerr << "Unable to create device enumerator" << std::endl;
        return;
    }

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr))
    {
        std::cerr << "Unable to get default audio endpoint" << std::endl;
        return;
    }

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&audioClient);
    if (FAILED(hr))
    {
        std::cerr << "Unable to activate audio client" << std::endl;
        return;
    }

    WAVEFORMATEX *waveFormat = nullptr;
    hr = audioClient->GetMixFormat(&waveFormat);
    if (FAILED(hr))
    {
        std::cerr << "Unable to get mix format" << std::endl;
        return;
    }

    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK,
                                 0, 0, waveFormat, nullptr);
    if (FAILED(hr))
    {
        std::cerr << "Unable to initialize audio client" << std::endl;
        return;
    }

    hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void **)&captureClient);
    if (FAILED(hr))
    {
        std::cerr << "Unable to get capture client" << std::endl;
        return;
    }

    audioClient->Start(); // Start capturing audio
    capturing = true;

    while (capturing)
    {
        UINT32 packetLength = 0;
        captureClient->GetNextPacketSize(&packetLength);
        while (packetLength != 0)
        {
            BYTE *data;
            UINT32 framesAvailable;
            DWORD flags;

            captureClient->GetBuffer(&data, &framesAvailable, &flags, nullptr, nullptr);
            UINT32 bytesToCapture = framesAvailable * waveFormat->nBlockAlign;

            audioData.insert(audioData.end(), data, data + bytesToCapture);
            captureClient->ReleaseBuffer(framesAvailable);
            captureClient->GetNextPacketSize(&packetLength);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep to reduce CPU usage
    }
    audioClient->Stop(); // Stop capturing audio
    CoTaskMemFree(waveFormat);
    audioClient->Release();
    device->Release();
    enumerator->Release();
}

// Function to stop audio capture
void StopCapture()
{
    capturing = false;
}

// Wrapper function to start audio capture from Node.js
void StartCaptureWrapper(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (capturing)
    {
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return; // Already capturing
    }
    std::thread captureThread(StartCapture);
    captureThread.detach();
    args.GetReturnValue().Set(Boolean::New(isolate, true));
}

// Wrapper function to stop audio capture from Node.js
void StopCaptureWrapper(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    StopCapture();
    args.GetReturnValue().Set(Boolean::New(isolate, true));
}

// Initialize the module and export the functions
void Initialize(Local<Object> exports)
{
    NODE_SET_METHOD(exports, "startCapture", StartCaptureWrapper);
    NODE_SET_METHOD(exports, "stopCapture", StopCaptureWrapper);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
