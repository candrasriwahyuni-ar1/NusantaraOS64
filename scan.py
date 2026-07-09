import pathlib
import gitignorefile
from datetime import datetime

def generate_project_context(root_path_str, output_file_name="project_context.txt"):
    root_path = pathlib.Path(root_path_str).resolve()
    # Menggunakan prefix \\?\ untuk menangani limit MAX_PATH di Windows jika diperlukan [5, 6]
    
    # Inisialisasi parser gitignore jika file .gitignore ada [4]
    matches_ignore = None
    if (root_path / ".gitignore").exists():
        matches_ignore = gitignorefile.parse(root_path / ".gitignore")

    with open(output_file_name, "w", encoding="utf-8") as f:
        # 1. Judul dan Metadata [7]
        f.write(f"PROJECT CONTEXT AGGREGATION\n")
        f.write(f"Generated on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Root Directory: {root_path}\n")
        f.write("-" * 50 + "\n\n")

        # 2. Struktur Folder (Directory Tree) [1, 7]
        f.write("DIRECTORY STRUCTURE:\n")
        file_list = []
        # Menggunakan Path.walk (tersedia di Python 3.12+) untuk traversal yang efisien [8]
        for dirpath, dirnames, filenames in root_path.walk(top_down=True):
            # Filter folder tersembunyi atau yang di-ignore [4, 9]
            dirnames[:] = [d for d in dirnames if not d.startswith('.')]
            if matches_ignore:
                dirnames[:] = [d for d in dirnames if not matches_ignore(str(dirpath / d))]

            depth = len(dirpath.relative_to(root_path).parts)
            indent = "  " * depth
            f.write(f"{indent}📁 {dirpath.name}/\n")
            
            for name in filenames:
                file_full_path = dirpath / name
                # Lewati file jika di-ignore oleh .gitignore atau file biner [2, 4]
                if matches_ignore and matches_ignore(str(file_full_path)):
                    continue
                if name.startswith('.') or name == output_file_name:
                    continue
                
                f.write(f"{indent}  📄 {name}\n")
                file_list.append(file_full_path)
        
        f.write("\n" + "=" * 50 + "\n\n")

        # 3. Indeks File [2, 7]
        f.write("FILE INDEX:\n")
        for i, path in enumerate(file_list, 1):
            f.write(f"{i}. {path.relative_to(root_path)}\n")
        f.write("\n" + "=" * 50 + "\n\n")

        # 4. Konten File dengan Pemisah Jelas [1, 9]
        for path in file_list:
            f.write(f"FILE: {path.relative_to(root_path)}\n")
            f.write("-" * 30 + "\n")
            try:
                # Membaca konten sebagai teks UTF-8 [9, 10]
                content = path.read_text(encoding="utf-8")
                f.write(content)
            except Exception as e:
                f.write(f"[Error reading file: {e}]")
            f.write("\n\n" + "#" * 50 + "\n\n")

    print(f"Sukses! Konteks proyek telah disimpan di {output_file_name}")

if __name__ == "__main__":
    # Masukkan path proyek Anda di sini
    generate_project_context(".")