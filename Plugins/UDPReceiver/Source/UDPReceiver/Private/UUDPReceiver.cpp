/**
 * Author: Tyler Durrant (github.com/YourGitHubUsername)
 * Original Author: Jake Wright (github.com/wjake)
 * Original Date: 2025-02-24
 * Modified Date: 2026-02-05
 * File: UDPReceiver.cpp
 * Class: UUDPReceiver
 * Description: Actor component for receiving and processing UDP messages with event-driven functionality.
 * Dependencies: Networking.h, Sockets.h, UdpSocketReceiver.h
 *
 * Notes:
 * - This file is based on the original implementation by Jake Wright.
 * - Modifications were made to adapt it for use in my project.
 * - Original source: https://github.com/wjake/<repo-name>
 */

#include "UUDPReceiver.h"

UUDPReceiver::UUDPReceiver()
{
    bAutoActivate = true;

    LastID = 0;
}

void UUDPReceiver::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoActivate)
    {
        StartReceiver();
    }
}

void UUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopReceiver();
}

UUDPReceiver* UUDPReceiver::StartReceiver()
{

    FIPv4Address Addr;

    FIPv4Address::Parse(IP, Addr);
    FIPv4Endpoint Endpoint(Addr, Port);

    // Create and configure the UDP socket
    UdpSocket = FUdpSocketBuilder(SocketName)
        .AsNonBlocking()
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithReceiveBufferSize(BufferSize)
        .Build();

    if (!UdpSocket)
    {
        UE_LOG(LogUDPReceiver, Error, TEXT("Failed to create UDP socket with Name: %s"), *SocketName);
        return nullptr;
    }

    // Initialise the UDP socket receiver
    UdpSocketReceiver = MakeShareable(new FUdpSocketReceiver(UdpSocket, FTimespan::FromMilliseconds(100), TEXT("UdpSocketReceiver")));
    UdpSocketReceiver->OnDataReceived().BindUObject(this, &UUDPReceiver::OnMessageReceived);
    UdpSocketReceiver->Start();
    UE_LOG(LogUDPReceiver, Warning, TEXT("UDP receiver started."));

    return this;
}

UUDPReceiver* UUDPReceiver::StopReceiver()
{
    if (UdpSocketReceiver.IsValid())
    {
        UdpSocketReceiver->Stop();
        UdpSocketReceiver = nullptr;
    }
    if (UdpSocket)
    {
        UdpSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UdpSocket);
        UdpSocket = nullptr;
    }

    return this;
}

UUDPReceiver* UUDPReceiver::UpdateConfig(const FString& NewIP, int32 NewPort, int32 NewBufferSize)
{
    IP = NewIP;
    Port = NewPort;
    BufferSize = NewBufferSize;

    UE_LOG(LogUDPReceiver, Log, TEXT("Updated Config - IP: %s, Port: %d, BufferSize: %d"), *IP, Port, BufferSize);

    return this;
}


bool UUDPReceiver::SendMessage(const FString& Message, const FString& ReceipientIP)
{
    bool bMessageSent = false;

    FIPv4Address Addr;
    if (!FIPv4Address::Parse(ReceipientIP, Addr))
    {
        UE_LOG(LogUDPReceiver, Error, TEXT("Invalid IP Address: %s"), *ReceipientIP);
        return bMessageSent;
    }

    FIPv4Endpoint Endpoint(Addr, Port);
    TArray<uint8> DataBytes;
    FTCHARToUTF8 Convert(*Message);
    DataBytes.Append(reinterpret_cast<const uint8*>(Convert.Get()), Convert.Length());

    int32 BytesSent = 0;
    if (UdpSocket->SendTo(DataBytes.GetData(), DataBytes.Num(), BytesSent, *Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogUDPReceiver, Log, TEXT("Sent reply: '%s' to %s"), *Message, *Endpoint.ToString());
        return bMessageSent = true;
    }
    else
    {
        UE_LOG(LogUDPReceiver, Error, TEXT("Failed to send reply to %s"), *Endpoint.ToString());
        return bMessageSent;
    }

}

void UUDPReceiver::OnMessageReceived(const TSharedPtr<FArrayReader, ESPMode::ThreadSafe>& Data, const FIPv4Endpoint& Endpoint)
{

    // Null-terminate the received data
    TArray<uint8> ReceivedDataArray = *Data;

    // -------------------Debugging examples-------------------
    // Check and print total data size
    //UE_LOG(LogTemp, Warning, TEXT("-----------Packet size: %d bytes-----------"), ReceivedDataArray.Num());

    // Create index to first data point, skipping data header
    const uint8* StartPoint = ReceivedDataArray.GetData() + 4;
    float* f = (float*)StartPoint;

    // Extract all points from the packet
    TArray<FLidarPoint> PacketPoints;
    int32 NumPoints = (ReceivedDataArray.Num() - 4) / sizeof(FLidarPoint);
    PacketPoints.SetNum(NumPoints);
    FMemory::Memcpy(PacketPoints.GetData(), StartPoint, NumPoints * sizeof(FLidarPoint));

    // Extract Frame ID from the packet
    FrameID = *reinterpret_cast<const uint32*>(ReceivedDataArray.GetData());

    if (FrameID > LastID || FramePoints.Num() == 20000)
    {
        //UE_LOG(LogTemp, Warning, TEXT("New Frame!"));

        // Push point list onto game thread and render
        AsyncTask(ENamedThreads::GameThread, [this, Frame = MoveTemp(WorkerFramePoints)]() mutable
            {
                FramePoints = MoveTemp(Frame);

                //UE_LOG(LogTemp, Warning, TEXT("Number of points in frame: %d"), FramePoints.Num()); //got rid of this might fix speed

                OnFrameReady.Broadcast(FramePoints);

            });

        // Have inside if to prevent rendering single packets if out of order
        LastID = FrameID;

        WorkerFramePoints.Reserve(20000);
    }

    //UE_LOG(LogTemp, Warning, TEXT("Worker --- Num: %d, Max: %d, Slack: %d"), WorkerFramePoints.Num(), WorkerFramePoints.Max(), WorkerFramePoints.GetSlack());

    WorkerFramePoints.Append(PacketPoints);

}