#include "librats.h"
#include <iostream>

int main() {
    librats::RatsClient client(8080);
    
    // Set up file transfer callbacks
    client.on_file_transfer_progress([](const librats::FileTransferProgress& progress) {
        std::cout << "📁 Transfer " << progress.transfer_id.substr(0, 8) 
                  << ": " << progress.get_completion_percentage() << "% complete"
                  << " (" << (progress.transfer_rate_bps / 1024) << " KB/s)" << std::endl;
    });
    
    client.on_file_transfer_completed([](const std::string& transfer_id, bool success, const std::string& error) {
        if (success) {
            std::cout << "✅ Transfer completed: " << transfer_id.substr(0, 8) << std::endl;
        } else {
            std::cout << "❌ Transfer failed: " << error << std::endl;
        }
    });
    
    // Auto-accept incoming file transfers
    client.on_file_transfer_request([](const std::string& peer_id, 
                                      const librats::FileMetadata& metadata, 
                                      const std::string& transfer_id) {
        std::cout << "📥 Incoming: " << metadata.filename 
                  << " (" << metadata.file_size << " bytes) from " << peer_id.substr(0, 8) << std::endl;
        return true; // Auto-accept
    });
    
    // Allow file requests from "shared" directory
    client.on_file_request([](const std::string& peer_id, const std::string& file_path, const std::string& transfer_id) {
        std::cout << "📤 Request: " << file_path << " from " << peer_id.substr(0, 8) << std::endl;
        return file_path.find("../") == std::string::npos; // Prevent path traversal
    });
    
    client.start();
    
    // Configure transfer settings
    librats::FileTransferConfig config;
    config.chunk_size = 64 * 1024;       // 64KB chunks
    config.max_concurrent_chunks = 4;    // 4 parallel chunks
    config.verify_checksums = true;      // Verify integrity
    client.set_file_transfer_config(config);
    
    // Example transfers (replace "peer_id" with actual peer ID)
    // std::string file_transfer = client.send_file("peer_id", "my_file.txt");
    // std::string dir_transfer = client.send_directory("peer_id", "./my_folder");
    // std::string file_request = client.request_file("peer_id", "remote_file.txt", "./downloaded_file.txt");
    
    std::cout << "File transfer ready. Connect peers and exchange files!" << std::endl;
    
    return 0;
}
