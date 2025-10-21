"""
A simple Tkinter GUI application to load an image, resize it, and convert it to a binary format (RGB565).
Ready to import in the esp32 eye project
"""

import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageTk
import struct

class ImageViewer:
    """
    A class to create a GUI for loading, resizing, and converting images to binary format.
    """
    def __init__(self, master):
        """
        Initializes the ImageViewer application.

        Args:
            master: The root Tkinter window.
        """
        self.master = master
        master.title("Image to Binary Converter")
        master.resizable(False, False) # Prevent window resizing

        # --- Image Attributes ---
        self.new_image_original_pil = None
        self.new_image_resized_pil = None
        self.resized_image_tk = None

        # --- Main Frame ---
        main_frame = tk.Frame(master, padx=10, pady=10)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # --- Image Converter Frame ---
        importer_frame = tk.LabelFrame(main_frame, text="Image Converter")
        importer_frame.pack(pady=10, padx=10, fill=tk.BOTH, expand=True)

        # --- Controls Frame ---
        importer_controls = tk.Frame(importer_frame)
        importer_controls.pack(pady=5, fill=tk.X)

        self.load_new_image_button = tk.Button(
            importer_controls, text="Load Image (PNG/BMP)", command=self.load_new_image
        )
        self.load_new_image_button.pack(side=tk.LEFT, padx=5)

        # --- Resizing Controls ---
        resize_frame = tk.Frame(importer_controls)
        resize_frame.pack(side=tk.LEFT, padx=20)

        tk.Label(resize_frame, text="W:").pack(side=tk.LEFT)
        self.new_width_var = tk.StringVar()
        self.new_width_entry = tk.Entry(
            resize_frame, textvariable=self.new_width_var, width=5
        )
        self.new_width_entry.pack(side=tk.LEFT)

        tk.Label(resize_frame, text="H:").pack(side=tk.LEFT, padx=(5, 0))
        self.new_height_var = tk.StringVar()
        self.new_height_entry = tk.Entry(
            resize_frame, textvariable=self.new_height_var, width=5
        )
        self.new_height_entry.pack(side=tk.LEFT)

        self.apply_resize_button = tk.Button(
            resize_frame, text="Apply Resize", command=self.resize_image, state=tk.DISABLED
        )
        self.apply_resize_button.pack(side=tk.LEFT, padx=5)

        self.generate_bin_button = tk.Button(
            importer_controls,
            text="Generate Binary File",
            command=self.generate_binary_file,
            state=tk.DISABLED,
        )
        self.generate_bin_button.pack(side=tk.RIGHT, padx=5)

        # --- Image Display Frame ---
        image_display_frame = tk.Frame(importer_frame)
        image_display_frame.pack(pady=10, fill=tk.BOTH, expand=True)

        resized_frame = tk.LabelFrame(image_display_frame, text="Resized Preview")
        resized_frame.pack(side=tk.RIGHT, padx=10, pady=5, fill=tk.BOTH, expand=True)
        self.resized_image_label = tk.Label(resized_frame)
        self.resized_image_label.pack(padx=5, pady=5)

    def load_new_image(self):
        """Loads a PNG or BMP image for processing."""
        file_path = filedialog.askopenfilename(
            filetypes=[("Image files", "*.png *.bmp")]
        )
        if not file_path:
            return

        try:
            self.new_image_original_pil = Image.open(file_path).convert("RGB")

            # Populate width/height fields
            width, height = self.new_image_original_pil.size
            self.new_width_var.set(str(width))
            self.new_height_var.set(str(height))

            # Automatically apply resize to show initial state
            self.resize_image()

            # Enable buttons now that an image is loaded
            self.apply_resize_button.config(state=tk.NORMAL)
            self.generate_bin_button.config(state=tk.NORMAL)

        except Exception as e:
            messagebox.showerror("Error", f"Could not load image:\n{e}")

    def resize_image(self):
        """Resizes the loaded image based on the entry fields and displays it."""
        if not self.new_image_original_pil:
            messagebox.showwarning("Warning", "No image loaded to resize.")
            return

        try:
            width = int(self.new_width_var.get())
            height = int(self.new_height_var.get())
        except ValueError:
            messagebox.showerror("Error", "Invalid width or height. Please enter numbers.")
            return

        self.new_image_resized_pil = self.new_image_original_pil.resize(
            (width, height), Image.Resampling.LANCZOS
        )

        # Display resized image
        self.resized_image_tk = ImageTk.PhotoImage(self.new_image_resized_pil)
        self.resized_image_label.config(image=self.resized_image_tk)
        self.resized_image_label.image = self.resized_image_tk

    def generate_binary_file(self):
        """Saves the currently resized image to a binary .bin file in RGB565 format."""
        if not self.new_image_original_pil:
            messagebox.showwarning("Warning", "No image loaded to save.")
            return

        file_path = filedialog.asksaveasfilename(
            defaultextension=".bin",
            filetypes=[("Binary files", "*.bin")],
            initialfile="image.bin",
        )

        if not file_path:
            return  # User cancelled

        try:
            img = self.new_image_resized_pil
            pixels = list(img.getdata())

            with open(file_path, "wb") as f:
                # Convert each RGB888 pixel to RGB565 and write to the binary file
                for r, g, b in pixels:
                    # RGB565 format: 5 bits for Red, 6 bits for Green, 5 bits for Blue
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    f.write(struct.pack("<H", rgb565))

            messagebox.showinfo("Success", f"Image saved to binary file:\n{file_path}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save binary file:\n{e}")

if __name__ == "__main__":
    root = tk.Tk()
    viewer = ImageViewer(root)
    root.mainloop()