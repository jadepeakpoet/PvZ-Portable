/* SPDX-License-Identifier: LGPL-3.0-or-later */
package io.github.wszqkzqk.pvzportable;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;
import androidx.documentfile.provider.DocumentFile;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

/** SAF paths are content URIs, so imports are copied to app-owned staging. */
final class TrainerFiles {
    private static final int OPEN_FILE = 18401, OPEN_DIRECTORY = 18402, EXPORT = 18403, EXPORT_DIRECTORY = 18404;
    private final Activity activity;
    private long ticket;
    private String result;
    private boolean busy;
    private File exportSource;
    private long copiedBytes;
    private int copiedEntries;

    TrainerFiles(Activity activity) { this.activity = activity; }

    void choose(int kind, long token) {
        activity.runOnUiThread(() -> {
            if (busy) { complete(token, ""); return; }
            ticket = token;
            if (kind == 1) {
                File base = new File(activity.getExternalFilesDir(null), "trainer/exports");
                if (!base.isDirectory() && !base.mkdirs()) { complete(token, ""); return; }
                complete(token, new File(base, "toolkit-" + UUID.randomUUID() + ".txt").getAbsolutePath());
                return;
            }
            try {
                busy = true;
                Intent intent = new Intent(kind == 2 ? Intent.ACTION_OPEN_DOCUMENT_TREE : Intent.ACTION_OPEN_DOCUMENT);
                if (kind != 2) { intent.addCategory(Intent.CATEGORY_OPENABLE); intent.setType("*/*"); }
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                activity.startActivityForResult(intent, kind == 2 ? OPEN_DIRECTORY : OPEN_FILE);
            } catch (RuntimeException e) { busy = false; complete(token, ""); error(e); }
        });
    }

    void share(String path) {
        activity.runOnUiThread(() -> {
            if (busy) return;
            File file = new File(path);
            if (!file.isFile() && !file.isDirectory()) return;
            try {
                busy = true; exportSource = file;
                Intent intent = new Intent(file.isDirectory() ? Intent.ACTION_OPEN_DOCUMENT_TREE : Intent.ACTION_CREATE_DOCUMENT);
                if (!file.isDirectory()) {
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("application/octet-stream");
                    intent.putExtra(Intent.EXTRA_TITLE, file.getName());
                }
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                activity.startActivityForResult(intent, file.isDirectory() ? EXPORT_DIRECTORY : EXPORT);
            } catch (RuntimeException e) { busy = false; exportSource = null; error(e); }
        });
    }

    synchronized String poll() { String value = result; result = null; return value; }
    private synchronized void complete(long token, String path) { result = token + "\n" + path; }

    boolean onResult(int request, int status, Intent data) {
        if (request != OPEN_FILE && request != OPEN_DIRECTORY && request != EXPORT && request != EXPORT_DIRECTORY) return false;
        final long currentTicket = ticket;
        if (status != Activity.RESULT_OK || data == null || data.getData() == null) {
            busy = false; exportSource = null;
            if (request == OPEN_FILE || request == OPEN_DIRECTORY) complete(currentTicket, "");
            return true;
        }
        final Uri uri = data.getData();
        final File exporting = exportSource;
        new Thread(() -> {
            File importRoot = null;
            try {
                copiedBytes = 0; copiedEntries = 0;
                if (request == EXPORT_DIRECTORY) {
                    DocumentFile tree = DocumentFile.fromTreeUri(activity, uri);
                    if (tree == null || exporting == null) throw new IOException("Cannot open export directory");
                    exportDocument(exporting, tree, 0);
                } else if (request == EXPORT) {
                    if (exporting == null) throw new IOException("No export file");
                    try (InputStream in = new FileInputStream(exporting);
                         OutputStream out = activity.getContentResolver().openOutputStream(uri, "w")) {
                        if (out == null) throw new IOException("Cannot open document");
                        copy(in, out);
                    }
                } else {
                    File root = new File(activity.getExternalFilesDir(null), "trainer/imports/" + UUID.randomUUID());
                    if (!root.mkdirs()) throw new IOException("Cannot create import directory");
                    importRoot = root;
                    DocumentFile selected = request == OPEN_DIRECTORY ? DocumentFile.fromTreeUri(activity, uri)
                            : DocumentFile.fromSingleUri(activity, uri);
                    if (selected == null) throw new IOException("Cannot open document");
                    File destination = new File(root, safeName(selected.getName()));
                    copyDocument(selected, destination, 0);
                    complete(currentTicket, destination.getAbsolutePath());
                }
            } catch (Exception e) {
                if (importRoot != null) deleteTree(importRoot);
                if (request == OPEN_FILE || request == OPEN_DIRECTORY) complete(currentTicket, "");
                error(e);
            } finally {
                activity.runOnUiThread(() -> { busy = false; exportSource = null; });
            }
        }, "PvZ-Trainer-Files").start();
        return true;
    }

    private static void deleteTree(File root) {
        File[] children = root.listFiles();
        if (children != null) for (File child : children) deleteTree(child);
        root.delete();
    }

    private static String safeName(String name) throws IOException {
        if (name == null || name.isEmpty() || name.equals(".") || name.equals("..")
                || name.contains("/") || name.contains("\\") || name.indexOf('\0') >= 0)
            throw new IOException("Invalid document name");
        return name;
    }
    private void copyDocument(DocumentFile source, File destination, int depth) throws IOException {
        if (depth > 32 || ++copiedEntries > 10000) throw new IOException("Import has too many entries");
        if (source.isDirectory()) {
            if (!destination.mkdirs()) throw new IOException("Cannot create directory");
            for (DocumentFile child : source.listFiles())
                copyDocument(child, new File(destination, safeName(child.getName())), depth + 1);
        } else {
            try (InputStream in = activity.getContentResolver().openInputStream(source.getUri());
                 OutputStream out = new FileOutputStream(destination)) {
                if (in == null) throw new IOException("Cannot read document");
                byte[] buffer = new byte[65536]; int count;
                while ((count = in.read(buffer)) != -1) {
                    copiedBytes += count;
                    if (copiedBytes > 512L * 1024 * 1024) throw new IOException("Import exceeds 512 MB");
                    out.write(buffer, 0, count);
                }
            }
        }
    }
    private void exportDocument(File source, DocumentFile parent, int depth) throws IOException {
        if (depth > 32 || ++copiedEntries > 10000) throw new IOException("Export has too many entries");
        String name = safeName(source.getName());
        if (parent.findFile(name) != null) throw new IOException("Destination already exists: " + name);
        if (source.isDirectory()) {
            DocumentFile directory = parent.createDirectory(name);
            if (directory == null) throw new IOException("Cannot create document directory");
            File[] children = source.listFiles();
            if (children == null) throw new IOException("Cannot read export directory");
            for (File child : children) exportDocument(child, directory, depth + 1);
        } else {
            copiedBytes += source.length();
            if (copiedBytes > 512L * 1024 * 1024) throw new IOException("Export exceeds 512 MB");
            DocumentFile file = parent.createFile("application/octet-stream", name);
            if (file == null) throw new IOException("Cannot create document");
            try (InputStream in = new FileInputStream(source);
                 OutputStream out = activity.getContentResolver().openOutputStream(file.getUri(), "w")) {
                if (out == null) throw new IOException("Cannot write document");
                copy(in, out);
            }
        }
    }
    private static void copy(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[65536]; int count;
        while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
    }
    private void error(Exception e) {
        Log.e("PvZTrainer", "Document operation failed", e);
        activity.runOnUiThread(() -> Toast.makeText(activity,
                "PvZ Toolkit: " + e.getMessage(), Toast.LENGTH_LONG).show());
    }
}
