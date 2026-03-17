#pragma once

#include "lib/runtime.h"
#include "tests.h"

// =============================================================================
// WebSocket Tests - WebSocketClient Implementation Validation
// Test Server: echo.websocket.org - WebSocket Echo Service
//
// Tests are consolidated to use minimal connections to avoid server rate limits.
// =============================================================================

class WebSocketTests
{
private:
	// Helper: verify echo response matches sent data
	static BOOL VerifyEcho(WebSocketClient &ws, Span<const CHAR> sent, WebSocketOpcode expectedOpcode, [[maybe_unused]] PCCHAR label)
	{
		auto readResult = ws.Read();
		if (!readResult)
		{
			LOG_ERROR("Failed to receive echo for %s (error: %e)", label, readResult.Error());
			return false;
		}

		WebSocketMessage &response = readResult.Value();

		if (response.Opcode != expectedOpcode)
		{
			LOG_ERROR("%s: unexpected opcode: expected %d, got %d", label, (UINT8)expectedOpcode, (UINT8)response.Opcode);
			return false;
		}

		if (response.Length != sent.Size() || Memory::Compare(response.Data, sent.Data(), sent.Size()) != 0)
		{
			LOG_ERROR("%s: echo response does not match sent data", label);
			return false;
		}

		return true;
	}

	// Helper: send data and verify echo
	static BOOL SendAndVerifyEcho(WebSocketClient &ws, Span<const CHAR> data, WebSocketOpcode opcode, PCCHAR label)
	{
		auto writeResult = ws.Write(data, opcode);
		if (!writeResult)
		{
			LOG_ERROR("Failed to send %s (error: %e)", label, writeResult.Error());
			return false;
		}

		return VerifyEcho(ws, data, opcode, label);
	}

	// Connection 1: All echo-based tests on a single connection
	static BOOL TestEchoSuite()
	{
		LOG_INFO("Connecting to echo.websocket.org (wss://)...");

		const CHAR wssUrl[] = "wss://echo.websocket.org/";
		auto createResult = WebSocketClient::Create(Span<const CHAR>(wssUrl, sizeof(wssUrl) - 1));
		if (!createResult)
		{
			LOG_ERROR("WebSocket connection failed (error: %e)", createResult.Error());
			return false;
		}
		WebSocketClient &ws = createResult.Value();
		LOG_INFO("  PASSED: Secure WebSocket connection (wss://)");

		// Discard initial server message
		auto initialMsg = ws.Read();
		if (initialMsg)
			LOG_INFO("Received initial server message (%d bytes), discarding", initialMsg.Value().Length);

		BOOL allPassed = true;

		// --- Text echo ---
		{
			LOG_INFO("Test: WebSocket Text Echo");
			const CHAR msg[] = "Hello, WebSocket!";
			if (SendAndVerifyEcho(ws, Span<const CHAR>(msg, sizeof(msg) - 1), WebSocketOpcode::Text, "text echo"))
				LOG_INFO("  PASSED: WebSocket text echo");
			else
			{
				LOG_ERROR("  FAILED: WebSocket text echo");
				allPassed = false;
			}
		}

		// --- Binary echo ---
		{
			LOG_INFO("Test: WebSocket Binary Echo");
			UINT8 binaryData[11];
			for (UINT32 i = 0; i < 5; i++)
				binaryData[i] = (UINT8)(i + 1);
			for (UINT32 i = 5; i < 11; i++)
				binaryData[i] = (UINT8)(0xAA + ((i - 5) * 0x11));

			if (SendAndVerifyEcho(ws, Span<const CHAR>((PCHAR)binaryData, sizeof(binaryData)), WebSocketOpcode::Binary, "binary echo"))
				LOG_INFO("  PASSED: WebSocket binary echo");
			else
			{
				LOG_ERROR("  FAILED: WebSocket binary echo");
				allPassed = false;
			}
		}

		// --- Multiple sequential messages ---
		{
			LOG_INFO("Test: Multiple Sequential Messages");
			const CHAR msg1[] = "First message";
			const CHAR msg2[] = "Second message";
			const CHAR msg3[] = "Third message";

			BOOL seqPassed = true;
			seqPassed = SendAndVerifyEcho(ws, Span<const CHAR>(msg1, sizeof(msg1) - 1), WebSocketOpcode::Text, "message 1") && seqPassed;
			seqPassed = SendAndVerifyEcho(ws, Span<const CHAR>(msg2, sizeof(msg2) - 1), WebSocketOpcode::Text, "message 2") && seqPassed;
			seqPassed = SendAndVerifyEcho(ws, Span<const CHAR>(msg3, sizeof(msg3) - 1), WebSocketOpcode::Text, "message 3") && seqPassed;

			if (seqPassed)
				LOG_INFO("  PASSED: Multiple messages");
			else
			{
				LOG_ERROR("  FAILED: Multiple messages");
				allPassed = false;
			}
		}

		// --- Large message ---
		{
			LOG_INFO("Test: Large Message Handling");
			UINT32 largeMessageSize = 1024;
			PCHAR largeMessage = new CHAR[largeMessageSize];
			if (!largeMessage)
			{
				LOG_ERROR("Failed to allocate memory for large message");
				allPassed = false;
			}
			else
			{
				for (UINT32 i = 0; i < largeMessageSize; i++)
					largeMessage[i] = 'A' + (i % 26);

				if (SendAndVerifyEcho(ws, Span<const CHAR>(largeMessage, largeMessageSize), WebSocketOpcode::Text, "large message"))
					LOG_INFO("  PASSED: Large message");
				else
				{
					LOG_ERROR("  FAILED: Large message");
					allPassed = false;
				}

				delete[] largeMessage;
			}
		}

		// --- Close handshake ---
		{
			LOG_INFO("Test: WebSocket Close Handshake");
			auto closeResult = ws.Close();
			if (closeResult)
				LOG_INFO("  PASSED: WebSocket close");
			else
			{
				LOG_ERROR("WebSocket close handshake failed (error: %e)", closeResult.Error());
				LOG_ERROR("  FAILED: WebSocket close");
				allPassed = false;
			}
		}

		return allPassed;
	}

public:
	// Run all WebSocket tests
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running WebSocket Tests...");
		LOG_INFO("  Test Server: echo.websocket.org (wss://)");

		if (!TestEchoSuite())
			allPassed = false;

		if (allPassed)
			LOG_INFO("All WebSocket tests passed!");
		else
			LOG_ERROR("Some WebSocket tests failed!");

		return allPassed;
	}
};
