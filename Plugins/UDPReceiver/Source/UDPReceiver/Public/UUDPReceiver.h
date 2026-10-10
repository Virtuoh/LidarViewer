/**
 * Author: ..... (github.com/YourGitHubUsername)
 * Original Author: Jake Wright (github.com/wjake)
 * Original Date: 2025-02-24
 * Modified Date: 2026-02-05
 * File: UDPReceiver.h
 * Class: UUDPReceiver
 * Description: Actor component for receiving and processing UDP messages with event-driven functionality.
 * Dependencies: Networking.h, Sockets.h, UdpSocketReceiver.h
 *
 * Notes:
 * - This file is based on the original implementation by Jake Wright.
 * - Modifications were made to adapt it for use in my project.
 * - Original source: https://github.com/wjake/<repo-name>
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Sockets.h"
#include "Async/Async.h"
#include "UUDPReceiver.generated.h"

USTRUCT()
struct FLidarPoint
{
    GENERATED_BODY()

    float X;
    float Y;
    float Z;
    float Intensity;
};

DEFINE_LOG_CATEGORY_STATIC(LogUDPReceiver, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMessageReceivedEvent,
    UUDPReceiver*, ReceiverInstance, const FString&, SenderIP, const FString&, ReceivedMessage);

DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnFrameUpdate,
    const TArray<FLidarPoint>&
);

UCLASS(ClassGroup = (Server), meta = (BlueprintSpawnableComponent))
class UUDPReceiver : public UActorComponent
{
    GENERATED_BODY()

public:
    UUDPReceiver();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Config")
    FString SocketName = TEXT("Socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Config")
    FString IP = TEXT("0.0.0.0");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Config")
    int32 Port = 8080;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Config")
    int32 BufferSize = 1500;

    /**
     * @brief Start the UDP receiver.
     * @return The UDP receiver instance.
     */
    UFUNCTION(BlueprintCallable, Category = "UDP Receiver")
    UUDPReceiver* StartReceiver();

    /**
     * @brief Stop the UDP receiver.
     * @return The UDP receiver instance.
     */
    UFUNCTION(BlueprintCallable, Category = "UDP Receiver")
    UUDPReceiver* StopReceiver();

    /**
     * @brief Update the UDP receiver configuration.
     * @param NewIP The new IP address.
     * @param NewPort The new port number.
     * @param NewBufferSize The new buffer size.
     * @return The UDP receiver instance.
     */
    UFUNCTION(BlueprintCallable, Category = "UDP Config")
    UUDPReceiver* UpdateConfig(const FString& NewIP, int32 NewPort, int32 NewBufferSize);

    /**
     * @brief Send a message to a specified recipient.
     * @param Message The message to send.
     * @param ReceipientIP The IP address of the recipient.
     * @return True if the message was sent successfully, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "UDP Receiver")
    bool SendMessage(const FString& Message, const FString& ReceipientIP);

    /**
     * @brief Event for handling received messages.
     */
    UPROPERTY(BlueprintAssignable, Category = "UDP Receiver")
    FMessageReceivedEvent OnMessageReceivedEvent;

    FOnFrameUpdate OnFrameReady;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    FSocket* UdpSocket;
    TSharedPtr<FUdpSocketReceiver> UdpSocketReceiver;

    // ID of current frame, owned by worker
    uint32 FrameID;

    // ID of previous frame, owned by worker
    uint32 LastID;

    // Array of current points compiled and owned by the worker
    TArray<FLidarPoint> WorkerFramePoints;

    TArray<FLidarPoint> FramePoints;

    /**
     * @brief Callback function for handling received UDP messages.
     * @param Data The received data.
     * @param Endpoint The endpoint from which the data was received.
     */
    void OnMessageReceived(const TSharedPtr<FArrayReader, ESPMode::ThreadSafe>& Data, const FIPv4Endpoint& Endpoint);
};