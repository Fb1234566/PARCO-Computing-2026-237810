import sys
import os
import numpy as np
from scipy.io import mmread, mmwrite
from scipy.sparse import csr_matrix

def analyze_matrix(filepath):
    """
    Loads a sparse matrix in Matrix Market format and prints its
    load-balancing statistics.
    """
    filename = os.path.basename(filepath)
    print(f"\n{'='*20} Analyzing: {filename} {'='*20}")

    # Load the matrix
    try:
        coo_matrix = mmread(filepath)
    except Exception as e:
        print(f"Error loading matrix {filename}: {e}")
        return

    # Convert to Compressed Sparse Row (CSR) format
    csr_mat = coo_matrix.tocsr()

    # --- Basic Statistics ---
    rows, cols = csr_mat.shape
    nnz = csr_mat.nnz

    print(f"Dimensions: {rows:,} x {cols:,}")
    print(f"Non-zeros (nnz): {nnz:,}")

    # --- Load Balance Statistics (NNZ per Row) ---
    if rows == 0:
        print("Matrix has no rows. Skipping row analysis.")
        print(f"{'='*(44 + len(filename))}")
        return

    # Get the number of non-zeros in each row
    nnz_per_row = np.diff(csr_mat.indptr)

    avg_nnz_row = nnz / rows
    max_nnz_row = nnz_per_row.max()
    min_nnz_row = nnz_per_row.min()
    std_dev_nnz_row = nnz_per_row.std()

    # Coefficient of variation (normalized measure of imbalance)
    coeff_of_variation = 0.0
    if avg_nnz_row > 0:
        coeff_of_variation = std_dev_nnz_row / avg_nnz_row

    print("\n--- Load Balance (NNZ per Row) ---")
    print(f"Average: {avg_nnz_row:,.2f}")
    print(f"Standard Deviation: {std_dev_nnz_row:,.2f}")
    print(f"Coefficient of Variation: {coeff_of_variation:.4f} (Higher = More Imbalanced)")
    print(f"Min Row NNZ: {min_nnz_row:,}")
    print(f"Max Row NNZ: {max_nnz_row:,}")

    # This ratio is a great indicator of imbalance
    if min_nnz_row > 0:
        print(f"Max/Min Ratio: {max_nnz_row / min_nnz_row:,.1f}")
    else:
        print("Contains empty rows (min row nnz = 0)")

    print(f"{'='*(44 + len(filename))}")


# --- Main execution ---
if __name__ == "__main__":
    # -----------------------------------------------------------------
    # --- SET YOUR DATASETS FOLDER PATH HERE ---
    # -----------------------------------------------------------------
    # This script assumes your 'datasets' folder is in the same
    # directory as the script. Change it if it's elsewhere.

    target_directory = "./datasets"

    # -----------------------------------------------------------------

    print(f"Scanning for .mtx files in: {os.path.abspath(target_directory)}\n")

    found_matrices = 0

    # Walk through the directory
    for root, dirs, files in os.walk(target_directory):
        for file in files:
            if file.endswith(".mtx"):
                full_path = os.path.join(root, file)
                try:
                    analyze_matrix(full_path)
                    found_matrices += 1
                except Exception as e:
                    print(f"\nFailed to analyze {full_path}: {e}")

    if found_matrices == 0:
        print("No .mtx files were found in the specified directory.")
        print("Please check the 'target_directory' variable in the script.")
    else:
        print(f"\nAnalysis complete. Found and analyzed {found_matrices} matrices.")
        print(f"\nAnalysis complete. Found and analyzed {found_matrices} matrices.")