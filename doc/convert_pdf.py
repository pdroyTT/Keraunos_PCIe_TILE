#!/usr/bin/env python3
"""
PDF to Markdown Converter
Converts PDF files from input_docs/ to Markdown in output_docs/
Supports local files and Google Drive links
"""

import os
import sys
import re
import tempfile
import urllib.parse

# Add current directory to Python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from preprocessing.document_preprocessor import DocumentPreprocessor

try:
    import requests
    HAS_REQUESTS = True
except ImportError:
    HAS_REQUESTS = False

try:
    from google.oauth2.credentials import Credentials
    from google_auth_oauthlib.flow import InstalledAppFlow
    from google.auth.transport.requests import Request
    from googleapiclient.discovery import build
    from googleapiclient.http import MediaIoBaseDownload
    import io
    HAS_GOOGLE_API = True
except ImportError:
    HAS_GOOGLE_API = False


def extract_google_drive_file_id(url):
    """
    Extract file ID from various Google Drive URL formats
    
    Supports:
    - https://drive.google.com/file/d/FILE_ID/view?usp=sharing
    - https://drive.google.com/open?id=FILE_ID
    - https://drive.google.com/uc?export=download&id=FILE_ID
    - FILE_ID (if just ID is provided)
    
    Returns:
        str: File ID or None if not a Google Drive link
    """
    # If it's just an ID (no http/https)
    if not url.startswith(('http://', 'https://')):
        # Check if it looks like a Google Drive ID (alphanumeric, ~33 chars)
        if re.match(r'^[a-zA-Z0-9_-]{25,}$', url):
            return url
    
    # Check if it's a Google Drive URL
    if 'drive.google.com' not in url:
        return None
    
    # Pattern 1: /file/d/FILE_ID/
    match = re.search(r'/file/d/([a-zA-Z0-9_-]+)', url)
    if match:
        return match.group(1)
    
    # Pattern 2: ?id=FILE_ID
    match = re.search(r'[?&]id=([a-zA-Z0-9_-]+)', url)
    if match:
        return match.group(1)
    
    return None


def authenticate_google_drive(credentials_file='credentials.json', token_file='token.json'):
    """
    Authenticate with Google Drive API
    
    Args:
        credentials_file: Path to OAuth2 credentials JSON file
        token_file: Path to store/load token
    
    Returns:
        googleapiclient.discovery.Resource: Authenticated Drive service
    """
    SCOPES = ['https://www.googleapis.com/auth/drive.readonly']
    creds = None
    
    # Load existing token
    if os.path.exists(token_file):
        creds = Credentials.from_authorized_user_file(token_file, SCOPES)
    
    # If there are no (valid) credentials available, let the user log in
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            if not os.path.exists(credentials_file):
                raise FileNotFoundError(
                    f"Credentials file not found: {credentials_file}\n"
                    "Please download credentials.json from Google Cloud Console.\n"
                    "See GOOGLE_DRIVE_SETUP.md for instructions."
                )
            flow = InstalledAppFlow.from_client_secrets_file(credentials_file, SCOPES)
            creds = flow.run_local_server(port=0)
        
        # Save the credentials for the next run
        with open(token_file, 'w') as token:
            token.write(creds.to_json())
    
    return build('drive', 'v3', credentials=creds)


def download_google_drive_file(file_id, output_path=None, use_auth=False, credentials_file='credentials.json'):
    """
    Download a file from Google Drive using its file ID
    
    Args:
        file_id: Google Drive file ID
        output_path: Path to save the file (optional, uses temp file if not provided)
        use_auth: Whether to use OAuth2 authentication (for private files)
        credentials_file: Path to OAuth2 credentials JSON file
    
    Returns:
        str: Path to downloaded file
    """
    if use_auth:
        # Use Google Drive API with authentication
        if not HAS_GOOGLE_API:
            raise ImportError(
                "Google API packages required for authenticated downloads.\n"
                "Install with: pip install google-auth google-auth-oauthlib google-auth-httplib2 google-api-python-client"
            )
        
        print(f"📥 Downloading from Google Drive (authenticated)...")
        print(f"   File ID: {file_id}")
        
        # Authenticate
        service = authenticate_google_drive(credentials_file)
        
        # Get file metadata
        file_metadata = service.files().get(fileId=file_id, fields='name, mimeType, size').execute()
        file_name = file_metadata.get('name', f'file_{file_id}')
        file_size = int(file_metadata.get('size', 0))
        
        print(f"   File name: {file_name}")
        if file_size > 0:
            print(f"   File size: {file_size:,} bytes ({file_size / 1024 / 1024:.2f} MB)")
        
        # Determine output path
        if output_path is None:
            temp_dir = tempfile.gettempdir()
            # Use original filename if available
            if file_name:
                output_path = os.path.join(temp_dir, file_name)
            else:
                output_path = os.path.join(temp_dir, f"gdrive_{file_id}.pdf")
        
        # Download the file
        request = service.files().get_media(fileId=file_id)
        downloaded = 0
        
        with open(output_path, 'wb') as f:
            downloader = MediaIoBaseDownload(f, request)
            done = False
            while done is False:
                status, done = downloader.next_chunk()
                if status:
                    downloaded = int(status.progress() * file_size) if file_size > 0 else 0
                    percent = status.progress() * 100
                    print(f"\r   Progress: {percent:.1f}% ({downloaded:,}/{file_size:,} bytes)", end='', flush=True)
        
        print()  # New line after progress
        print(f"✅ Downloaded: {output_path} ({os.path.getsize(output_path):,} bytes)")
        
        return output_path
    
    else:
        # Use public download method (original implementation)
        if not HAS_REQUESTS:
            raise ImportError("requests package is required for Google Drive downloads. Install with: pip install requests")
        
        # Google Drive direct download URL
        download_url = f"https://drive.google.com/uc?export=download&id={file_id}"
        
        print(f"📥 Downloading from Google Drive (public)...")
        print(f"   File ID: {file_id}")
        
        # Create session to handle cookies
        session = requests.Session()
        
        # First request to get download confirmation (for large files)
        response = session.get(download_url, stream=True, allow_redirects=True)
        
        # Check if we got a virus scan warning page
        if 'virus scan warning' in response.text.lower() or 'download anyway' in response.text.lower():
            # Extract the confirm token
            confirm_match = re.search(r'confirm=([a-zA-Z0-9_-]+)', response.text)
            if confirm_match:
                confirm_token = confirm_match.group(1)
                download_url = f"https://drive.google.com/uc?export=download&id={file_id}&confirm={confirm_token}"
                response = session.get(download_url, stream=True)
        
        # Check if download was successful
        if response.status_code != 200:
            # If public download fails, suggest using authentication
            if response.status_code == 403:
                raise Exception(
                    f"Access denied (403). This file requires authentication.\n"
                    "Try using --auth flag: python convert_pdf.py FILE_ID --auth"
                )
            raise Exception(f"Failed to download file. Status code: {response.status_code}")
        
        # Determine output path
        if output_path is None:
            # Use temp file
            temp_dir = tempfile.gettempdir()
            output_path = os.path.join(temp_dir, f"gdrive_{file_id}.pdf")
        
        # Download the file
        total_size = int(response.headers.get('content-length', 0))
        downloaded = 0
        
        with open(output_path, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                if chunk:
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total_size > 0:
                        percent = (downloaded / total_size) * 100
                        print(f"\r   Progress: {percent:.1f}% ({downloaded:,}/{total_size:,} bytes)", end='', flush=True)
        
        print()  # New line after progress
        print(f"✅ Downloaded: {output_path} ({downloaded:,} bytes)")
        
        return output_path


def resolve_input_file(input_source, use_auth=False, credentials_file='credentials.json'):
    """
    Resolve input source to a local file path
    
    Handles:
    - Local file paths
    - Google Drive URLs
    - Google Drive file IDs
    
    Args:
        input_source: File path, Google Drive URL, or file ID
        use_auth: Whether to use OAuth2 authentication for Google Drive
        credentials_file: Path to OAuth2 credentials JSON file
    
    Returns:
        tuple: (local_file_path, is_temporary)
    """
    # Check if it's a local file
    if os.path.exists(input_source):
        return input_source, False
    
    # Check if it's a Google Drive link
    file_id = extract_google_drive_file_id(input_source)
    
    if file_id:
        # Download from Google Drive
        temp_file = download_google_drive_file(file_id, use_auth=use_auth, credentials_file=credentials_file)
        return temp_file, True  # Mark as temporary
    
    # Check if it's a URL (but not Google Drive)
    if input_source.startswith(('http://', 'https://')):
        raise ValueError(f"Unsupported URL format. Only Google Drive links are supported. Got: {input_source}")
    
    # If we get here, assume it's a local path that doesn't exist
    raise FileNotFoundError(f"File not found: {input_source}")


def convert_pdf_to_markdown(input_file, output_dir="output_docs", cleanup_temp=True, use_auth=False, credentials_file='credentials.json'):
    """
    Convert a PDF file to Markdown format
    
    Supports:
    - Local file paths
    - Google Drive URLs
    - Google Drive file IDs
    
    Args:
        input_file: Path to the input PDF file, Google Drive URL, or file ID
        output_dir: Directory to save the output Markdown file
        cleanup_temp: Whether to delete temporary downloaded files
        use_auth: Whether to use OAuth2 authentication for Google Drive files
        credentials_file: Path to OAuth2 credentials JSON file
    
    Returns:
        dict: Conversion result with success status and output path
    """
    print("=" * 70)
    print("📄 PDF to Markdown Converter")
    print("=" * 70)
    print()
    
    # Resolve input (download from Google Drive if needed)
    try:
        local_file, is_temp = resolve_input_file(input_file, use_auth=use_auth, credentials_file=credentials_file)
    except FileNotFoundError as e:
        print(f"❌ Error: {e}")
        return {"success": False, "error": str(e)}
    except Exception as e:
        print(f"❌ Error resolving input: {e}")
        return {"success": False, "error": str(e)}
    
    print(f"📂 Input source: {input_file}")
    if is_temp:
        print(f"📥 Downloaded to: {local_file}")
    print(f"📁 Output dir:  {output_dir}")
    print()
    
    # Initialize the document preprocessor
    print("🚀 Initializing Document Preprocessor Agent...")
    agent = DocumentPreprocessor(output_dir=output_dir)
    print("✅ Agent initialized")
    print()
    
    # Convert the document
    print("🔄 Converting PDF to Markdown...")
    print(f"   Processing: {os.path.basename(local_file)}")
    print()
    
    try:
        result = agent.convert_document(local_file)
    finally:
        # Clean up temporary file if it was downloaded
        if is_temp and cleanup_temp and os.path.exists(local_file):
            try:
                os.remove(local_file)
                print(f"\n🧹 Cleaned up temporary file: {local_file}")
            except Exception as e:
                print(f"\n⚠️  Warning: Could not delete temporary file: {e}")
    
    # Display results
    print("-" * 70)
    if result["success"]:
        print("✅ CONVERSION SUCCESSFUL!")
        print()
        print(f"📝 Output file:     {result['output_path']}")
        print(f"📋 File type:       {result['file_type']}")
        print()
        print("📊 Metadata:")
        for key, value in result['metadata'].items():
            print(f"   • {key}: {value}")
        print()
        
        # Show file size
        output_size = os.path.getsize(result['output_path'])
        input_size = os.path.getsize(local_file)
        print(f"📏 Input size:      {input_size:,} bytes ({input_size / 1024 / 1024:.2f} MB)")
        print(f"📏 Output size:     {output_size:,} bytes ({output_size / 1024:.2f} KB)")
        print()
        
        # Show preview
        print("👀 Preview (first 500 characters):")
        print("=" * 70)
        with open(result['output_path'], 'r', encoding='utf-8') as f:
            preview = f.read(500)
            print(preview)
            if len(preview) >= 500:
                print("\n... (truncated)")
        print("=" * 70)
        
    else:
        print("❌ CONVERSION FAILED!")
        print()
        print(f"Error: {result['error']}")
        if 'supported_formats' in result:
            print(f"\nSupported formats: {', '.join(result['supported_formats'])}")
    
    print()
    print("=" * 70)
    
    return result


def main():
    """Main function"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Convert PDF files to Markdown format. Supports local files and Google Drive links.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Convert local PDF file
  python convert_pdf.py input_docs/spec.pdf
  
  # Convert from Google Drive URL
  python convert_pdf.py "https://drive.google.com/file/d/FILE_ID/view?usp=sharing"
  
  # Convert from Google Drive file ID
  python convert_pdf.py FILE_ID
  
  # Specify output directory
  python convert_pdf.py input_docs/spec.pdf --output-dir my_output
  
  # Keep downloaded file (don't auto-delete)
  python convert_pdf.py "https://drive.google.com/file/d/FILE_ID/view" --keep-temp
        """
    )
    
    parser.add_argument(
        "input_source",
        help="Input PDF file path, Google Drive URL, or Google Drive file ID"
    )
    parser.add_argument(
        "--output-dir",
        default="output_docs",
        help="Output directory for Markdown file (default: output_docs)"
    )
    parser.add_argument(
        "--keep-temp",
        action="store_true",
        help="Keep temporary downloaded files (don't auto-delete)"
    )
    parser.add_argument(
        "--auth",
        action="store_true",
        help="Use OAuth2 authentication for Google Drive (required for private files)"
    )
    parser.add_argument(
        "--credentials",
        default="credentials.json",
        help="Path to Google OAuth2 credentials JSON file (default: credentials.json)"
    )
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Convert the PDF
    result = convert_pdf_to_markdown(
        args.input_source,
        args.output_dir,
        cleanup_temp=not args.keep_temp,
        use_auth=args.auth,
        credentials_file=args.credentials
    )
    
    # Exit with appropriate status code
    sys.exit(0 if result["success"] else 1)


if __name__ == "__main__":
    main()

