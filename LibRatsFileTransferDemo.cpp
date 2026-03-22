#include "librats.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace librats;

int main() {
	// Create client
	RatsClient client(8080, 10, "169.254.242.80");

	// Configure file transfers
	FileTransferConfig config;
	config.chunk_size = 1024 * 1024;      // 64KB chunks
	config.max_concurrent_chunks = 8;    // 4 parallel transfers
	config.verify_checksums = false;      // Verify integrity
	config.allow_resume = false;          // Allow resume
	client.set_file_transfer_config(config);

	// Set up transfer event handlers
	client.on_file_transfer_request([&client](const std::string& peer_id,
		const FileMetadata& metadata,
		const std::string& transfer_id) -> bool {
			std::cout << "📁 Incoming file transfer request:" << std::endl;
			std::cout << "   From: " << peer_id << std::endl;
			std::cout << "   File: " << metadata.filename << std::endl;
			std::cout << "   Size: " << metadata.file_size << " bytes" << std::endl;

			// Auto-accept and save to downloads folder
			std::string local_path = "./downloads/" + metadata.filename;
			client.accept_file_transfer(transfer_id, local_path);
			std::cout << "✅ Transfer accepted -> " << local_path << std::endl;
			return true;
		});

	client.on_file_transfer_progress([](const FileTransferProgress& progress) {
		std::cout << "📊 " << progress.filename << ": "
			<< std::fixed << std::setprecision(1)
			<< progress.get_completion_percentage() << "% "
			<< "(" << progress.transfer_rate_bps / 1024 << " KB/s)" << std::endl;
		});

	client.on_file_transfer_completed([](const std::string& transfer_id,
		bool success,
		const std::string& error_message) {
			if (success) {
				std::cout << "✅ Transfer completed: " << transfer_id << std::endl;
			}
			else {
				std::cout << "❌ Transfer failed: " << error_message << std::endl;
			}
		});

	// Start the client
	if (!client.start()) {
		std::cerr << "Failed to start client" << std::endl;
		return 1;
	}

	client.start_mdns_discovery();

	std::cout << "🚀 RatsClient started on port 8080" << std::endl;
	std::cout << "📁 File transfer system ready" << std::endl;
	std::cout << "Commands:" << std::endl;
	std::cout << "  connect <ip> <port>            - Connect to an ip" << std::endl;
	std::cout << "  send <peer_id> <file_path>     - Send a file" << std::endl;
	std::cout << "  senddir <peer_id> <dir_path>   - Send a directory" << std::endl;
	std::cout << "  list                           - List active transfers" << std::endl;
	std::cout << "  stats                          - Show transfer statistics" << std::endl;
	std::cout << "  quit                           - Exit" << std::endl;

	// Command loop
	std::string command;
	while (std::getline(std::cin, command)) {
		if (command == "quit") {
			break;
		}
		else if (command == "list") {
			auto transfers = client.get_active_file_transfers();
			std::cout << "Active transfers (" << transfers.size() << "):" << std::endl;
			for (const auto& transfer : transfers) {
				std::cout << "  " << transfer->transfer_id << " - "
					<< transfer->filename << " ("
					<< transfer->get_completion_percentage() << "%)" << std::endl;
			}
		}
		else if (command == "stats") {
			auto stats = client.get_file_transfer_statistics();
			std::cout << "Transfer Statistics:" << std::endl;
			std::cout << "  Bytes sent: " << stats["total_bytes_sent"] << std::endl;
			std::cout << "  Bytes received: " << stats["total_bytes_received"] << std::endl;
			std::cout << "  Files sent: " << stats["total_files_sent"] << std::endl;
			std::cout << "  Files received: " << stats["total_files_received"] << std::endl;
			std::cout << "  Success rate: " << stats["success_rate"] << std::endl;
		}
		else if (command.substr(0, 4) == "send") {
			// Parse send command
			size_t first_space = command.find(' ', 5);
			size_t second_space = command.find(' ', first_space + 1);

			if (first_space != std::string::npos && second_space != std::string::npos) {
				std::string peer_id = command.substr(5, first_space - 5);
				std::string file_path = command.substr(first_space + 1);

				std::string transfer_id = client.send_file(peer_id, file_path);
				if (!transfer_id.empty()) {
					std::cout << "📤 Transfer initiated: " << transfer_id << std::endl;
				}
				else {
					std::cout << "❌ Failed to initiate transfer" << std::endl;
				}
			}
			else {
				std::cout << "Usage: send <peer_id> <file_path>" << std::endl;
			}
		}
		else if (command.substr(0, 7) == "senddir") {
			// Parse senddir command  
			size_t first_space = command.find(' ', 8);
			size_t second_space = command.find(' ', first_space + 1);

			if (first_space != std::string::npos && second_space != std::string::npos) {
				std::string peer_id = command.substr(8, first_space - 8);
				std::string dir_path = command.substr(first_space + 1);

				std::string transfer_id = client.send_directory(peer_id, dir_path);
				if (!transfer_id.empty()) {
					std::cout << "📁 Directory transfer initiated: " << transfer_id << std::endl;
				}
				else {
					std::cout << "❌ Failed to initiate directory transfer" << std::endl;
				}
			}
			else {
				std::cout << "Usage: senddir <peer_id> <dir_path>" << std::endl;
			}
		}
		else if (command.substr(0, 7) == "connect") {
			// parse connect command
			size_t first_space = command.find(' ');
			std::cout << first_space << std::endl;
			size_t second_space = command.find(' ', first_space + 1);
			std::cout << second_space << std::endl;
			if (first_space != std::string::npos && second_space != std::string::npos) {
				std::string ip = command.substr(8, second_space - first_space);
				std::cout << ip << std::endl;
				std::string portStr = command.substr(second_space + 1);
				std::cout << portStr << std::endl;
				std::cout << "IP: " << ip << " Port: " << portStr << std::endl;

				auto portInt = std::stoi(portStr);

				if (client.connect_to_peer(ip, portInt))
				{
					std::this_thread::sleep_for(std::chrono::seconds(1));
					std::cout << "Connected to peer " << ip << "Peer Count: " << client.get_peer_count() << std::endl;
					auto peers = client.get_all_peers();
					for (auto& peer : peers)
					{
						std::cout << peer.peer_id << std::endl;
					}
				}
				else {
					std::cout << "Did not connect" << std::endl;
				}
			}
			else {
				std::cout << "got here" << std::endl;
			}

		}

		std::cout << "Shutting down..." << std::endl;
		std::cin.get();
		client.stop();
		return 0;

	}








	//librats::RatsClient client(8080);

	//// Set up file transfer callbacks
	//client.on_file_transfer_progress([](const librats::FileTransferProgress& progress) {
	//	std::cout << "📁 Transfer " << progress.transfer_id.substr(0, 8)
	//		<< ": " << progress.get_completion_percentage() << "% complete"
	//		<< " (" << (progress.transfer_rate_bps / 1024) << " KB/s)" << std::endl;
	//	});

	//client.on_file_transfer_completed([](const std::string& transfer_id, bool success, const std::string& error) {
	//	if (success) {
	//		std::cout << "✅ Transfer completed: " << transfer_id.substr(0, 8) << std::endl;
	//	}
	//	else {
	//		std::cout << "❌ Transfer failed: " << error << std::endl;
	//	}
	//	});

	//// Auto-accept incoming file transfers
	//client.on_file_transfer_request([](const std::string& peer_id,
	//	const librats::FileMetadata& metadata,
	//	const std::string& transfer_id) {
	//		std::cout << "📥 Incoming: " << metadata.filename
	//			<< " (" << metadata.file_size << " bytes) from " << peer_id.substr(0, 8) << std::endl;
	//		return true; // Auto-accept
	//	});

	//// Allow file requests from "shared" directory
	//client.on_file_request([](const std::string& peer_id, const std::string& file_path, const std::string& transfer_id) {
	//	std::cout << "📤 Request: " << file_path << " from " << peer_id.substr(0, 8) << std::endl;
	//	return file_path.find("../") == std::string::npos; // Prevent path traversal
	//	});

	//// Set callback for discovered services
	//client.set_mdns_callback([](const std::string& ip, int port, const std::string& service_name) {
	//	std::cout << "Discovered librats service: " << service_name
	//		<< " at " << ip << ":" << port << std::endl;
	//	});

	//client.start();

	//// Configure transfer settings
	//librats::FileTransferConfig config;
	//config.chunk_size = 1024 * 1024;       // 64KB chunks
	//config.max_concurrent_chunks = 8;    // 4 parallel chunks
	//config.verify_checksums = false;      // Verify integrity
	//client.set_file_transfer_config(config);
	//

	////// Start mDNS discovery with custom TXT records
	////std::map<std::string, std::string> txt_records;
	////txt_records["version"] = "1.0";
	////txt_records["features"] = "encryption,dht";
	////txt_records["max_peers"] = "10";

	////client.start_mdns_discovery("my-node", txt_records);

	//if (client.connect_to_peer("192.168.0.178", 4496)) {
	//	std::this_thread::sleep_for(std::chrono::seconds(1));
	//	std::cout << "Connected to peer 192.168.0.178" << "Peer Count: " << client.get_peer_count() << std::endl;
	//	client.broadcast_string_to_peers("Hello from here");
	//	auto peers = client.get_all_peers();
	//	for (auto& peer : peers)
	//	{
	//		auto targetPeer = peer;
	//		std::string transfer_id = client.send_file(targetPeer.peer_id, "C:\\TestSync\\NewZealand4kTestClip.mov");

	//		std::cout << "File transfer id: " << transfer_id << std::endl;
	//	}
	//}
	//else {
	//	std::cout << "Did not connect" << std::endl;
	//}


	//// Example transfers (replace "peer_id" with actual peer ID)
	//// std::string dir_transfer = client.send_directory("peer_id", "./my_folder");
	//// std::string file_request = client.request_file("peer_id", "remote_file.txt", "./downloaded_file.txt");

	//std::cout << "File transfer ready. Connect peers and exchange files!" << std::endl;

	//std::cin.get();

	//return 0;
}
