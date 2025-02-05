#include "git/addurldialog.h"
#include "git/commitdialog.h"
#include "git/credentialsdialog.h"
#include "git/remotenamedialog.h"
#include <git/git.h>
#include <git/ui_git.h>
#include <git2.h>
#include <iostream>
#include <qdir.h>
#include <git/git_util.h>
#include <cstring>
#ifndef _WIN32
# include <unistd.h>
#endif
#include <errno.h>

int credentialsCallback(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload);
// int fetchhead_cb(const char *ref_name, const char *remote_url, const git_oid *oid, unsigned int is_merge, void *payload);
int print_matched_cb(const char *path, const char *matched_pathspec, void *payload);
bool check_lg2(int error, const char *message, const char *extra);
static int readline(char **out);
static int ask(char **out, const char *prompt, char optional, bool secure);
int cred_acquire_cb(git_credential **out, const char *url, const char *username_from_url,
                    unsigned int allowed_types,
                    void *payload);
bool match_arg(int *out, struct args_info *args, const char *opt);
static int progress_cb(const char *str, int len, void *data);
static int update_cb(const char *refname, const git_oid *a, const git_oid *b, void *data);
static int transfer_progress_cb(const git_indexer_progress *stats, void *payload);

Git::Git(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::Git)
{
    statusBar = new QStatusBar();

    git_libgit2_init();
    repoPath = QDir::currentPath() + "/vagyojaka-git";
    init();
}

void Git::init()
{
    QDir repoDir(repoPath);
    if(!repoDir.exists()) {
        qDebug() << "Initialing" << Qt::endl;
        repoDir.mkpath(repoPath);
    }

    if (!doesRepositoryExist(repoPath.toUtf8())) {

        int error = git_repository_init(&repo, repoPath.toUtf8(), 0);
        if (error < 0) {
            const git_error* e = giterr_last();
            qCritical() << "Error initializing repository:" << error << e->message;
            return;
        }

        qDebug() << "Repository initialized at:" << repoPath;

    } else {
        qDebug() << "Repository already exists at:" << repoPath << Qt::endl;
        qDebug() << "Opening Repository\n";

        int error = git_repository_open(&repo, repoPath.toUtf8());
        if (error < 0) {
            const git_error* e = giterr_last();
            qCritical() << "Error opening repository:" << error << e->message;
            git_libgit2_shutdown();
            return;
        }
        qDebug() << "Opening Repository was successful\n";
    }


}

void Git::add()
{
    // git_index *index = nullptr;
    // int error = git_repository_index(&index, repo);
    // if (error < 0) {
    //     const git_error* e = giterr_last();
    //     qCritical() << "Error getting repository index:" << error << e->message;
    //     return;
    // }

    // error = git_index_add_all(index, nullptr, 0, nullptr, nullptr);
    // if (error < 0) {
    //     const git_error* e = giterr_last();
    //     qCritical() << "Error adding files to index:" << error << e->message;
    //     git_index_free(index);
    //     return;
    // }

    // git_index_write(index);
    // git_index_free(index);

    // qDebug() << "Files added to the index.";

    git_index_matched_path_cb matched_cb = NULL;
    git_index *index;
    git_strarray array = {0};
    struct index_options options = {0};
    options.mode = INDEX_ADD;
    options.verbose = 1;
    options.dry_run = 0;
    options.add_update = 0;
    options.repo = repo;

    matched_cb = print_matched_cb;
    /* Grab the repository's index. */
    check_lg2(git_repository_index(&index, repo), "Could not open repository index", NULL);
    git_index_add_all(index, nullptr, 0, matched_cb, &options);


}

void Git::commit()
{

    git_oid *treeId, commitId, *parentId;

    git_oid commit_oid,tree_oid;
    git_tree *tree;
    git_index *index;
    git_object *parent = nullptr;
    git_reference *ref = nullptr;
    git_signature *author_signature;
    git_signature *committer_signature;

    int error = git_repository_index(&index, repo);
    if (error < 0) {
        const git_error* e = giterr_last();
        qCritical() << "Error getting repository index:" << error << e->message;
        return;
    }

    error = git_revparse_ext(&parent, &ref, repo, "HEAD");
    if (error == GIT_ENOTFOUND) {
        const git_error* err = giterr_last();
        qCritical() << "HEAD not found. Creating first commit: " << error << err->message;
    } else if (error != 0) {
        const git_error *err = git_error_last();
        if (err) qCritical() << "ERROR: " << err->klass << " Message: " << err->message;
        else qCritical() << "ERROR: " << error << "no detailed info\n";
    }
    if(check_lg2(git_repository_index(&index, repo), "Could not open repository index", nullptr)) {
        return;
    }
    if(check_lg2(git_index_write_tree(&tree_oid, index), "Could not write tree", nullptr)) {
        return;
    }

    if(check_lg2(git_index_write(index), "Could not write index", nullptr)) {
        return;
    }

    if(check_lg2(git_tree_lookup(&tree, repo, &tree_oid), "Error looking up tree", nullptr)) {
        return;
    }

    // if(check_lg2(git_signature_default(&signature, repo), "Error creating signature", nullptr)) {
    //     return;
    // }

    CommitDialog dialog = CommitDialog(this, "Author", false);
    QString author_name;
    QString author_email;

    if (dialog.exec() == QDialog::Accepted) {

        author_name = dialog.getUsername();
        author_email = dialog.getEmail();
    } else {

    }
    CommitDialog dialog2 = CommitDialog(this, "Committer", true);

    QString committer_name;
    QString committer_email;
    QString commit_message;
    if (dialog2.exec() == QDialog::Accepted) {
        committer_name = dialog2.getUsername();
        committer_email =  dialog2.getEmail();
        commit_message = dialog2.getCommitMessage();

    } else {

    }

    // Get current timestamp
    QDateTime currentTimestamp = QDateTime::currentDateTime();
    time_t currentTimestampInSeconds = currentTimestamp.toSecsSinceEpoch();

    // Create author signature
        if (check_lg2(
                git_signature_new(&author_signature, author_name.toStdString().c_str(), author_email.toStdString().c_str(), currentTimestampInSeconds, /*offset=*/0),
                "Error creating author signature", nullptr)) {
        return;
    }

    // Create committer signature
    if (check_lg2(
            git_signature_new(&committer_signature, committer_name.toStdString().c_str(), committer_email.toStdString().c_str(), currentTimestampInSeconds, /*offset=*/0),
            "Error creating committer signature", nullptr)) {
        git_signature_free(author_signature);
        return;
    }
    if(check_lg2(git_commit_create_v(
                  &commit_oid,
                  repo,
                  "HEAD",
                  author_signature,
                  committer_signature,
                  NULL,
                  commit_message.toStdString().c_str(),
                  tree,
                      parent ? 1 : 0, parent), "Error creating commit", NULL)) {
        return;
    }

    git_index_free(index);
    git_signature_free(committer_signature);
    git_signature_free(author_signature);
    git_tree_free(tree);
    git_object_free(parent);
    git_reference_free(ref);

}

void Git::addRemoteUrl()
{
    AddUrlDialog addUrlDialog = AddUrlDialog(this);

    const char* remoteName = "";
    const char* remoteUrl = "";

    if (addUrlDialog.exec() == QDialog::Accepted) {
        remoteName = strdup(addUrlDialog.getRemoteName().toStdString().c_str());
        remoteUrl = strdup(addUrlDialog.getRemoteURL().toStdString().c_str());
    }
    git_remote *remote = nullptr;

    // Check if the remote already exists
    int error = git_remote_lookup(&remote, repo, remoteName);

    if (error == 0) {
        // Remote exists, edit its URL
        error = git_remote_set_url(repo, remoteName, remoteUrl);

        if (error < 0) {
            const git_error* e = giterr_last();
            qCritical() << "Error setting remote URL:" << error << e->message;
        } else {
            qDebug() << "Remote '" << remoteName << "' URL updated successfully.";
        }

        git_remote_free(remote);
    } else {
        // Remote does not exist, create a new one
        error = git_remote_create(&remote, repo, remoteName, remoteUrl);

        if (error < 0) {
            const git_error* e = giterr_last();
            qCritical() << "Error adding remote:" << error << e->message;
        } else {
            qDebug() << "Remote '" << remoteName << "' added successfully with URL:" << remoteUrl;
        }
    }
}

void Git::push()
{
    git_push_options options;
    git_remote_callbacks callbacks;
    git_remote* remote = nullptr;
    // char *refspec = const_cast<char*>("refs/heads/master");
    // const git_strarray refspecs = {
    //     &refspec,
    //     1
    // };
    QString remote_name = "";
    RemoteNameDialog remoteNameDialog = RemoteNameDialog(this);

    if (remoteNameDialog.exec() == QDialog::Accepted) {
        remote_name = remoteNameDialog.getRemoteName();
    } else {
        return;
    }
    check_lg2(git_remote_lookup(&remote, repo, strdup(remote_name.toStdString().c_str())), "Unable to lookup remote", nullptr);

    check_lg2(git_remote_init_callbacks(&callbacks, GIT_REMOTE_CALLBACKS_VERSION),
              "Error initializing remote callbacks", nullptr);

    // CredentialsDialog* credentialsDialog = new CredentialsDialog(this);
    // callbacks.credentials = credentialsCallback;
    callbacks.credentials = cred_acquire_cb;

    check_lg2(git_push_options_init(&options, GIT_PUSH_OPTIONS_VERSION ), "Error initializing push", NULL);
    options.callbacks = callbacks;

    // Get the current branch's refspec dynamically.
    git_reference* headRef = nullptr;
    check_lg2(git_repository_head(&headRef, repo), "Unable to get HEAD reference", nullptr);

    const char* branchName = git_reference_shorthand(headRef);
    QString refspecStr = "refs/heads/" + QString(branchName);
    char* refspec = strdup(refspecStr.toUtf8().constData());

    // Free the HEAD reference.
    git_reference_free(headRef);

    const git_strarray refspecs = {
        &refspec,
        1
    };

    if(check_lg2(git_remote_push(remote, &refspecs, &options), "Error pushing", NULL)) {
        return;
    }


}

void Git::pull()
{

    git_remote *remote = NULL;
    const git_indexer_progress *stats;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

    QString remote_name = "";
    RemoteNameDialog remoteNameDialog = RemoteNameDialog(this);

    if (remoteNameDialog.exec() == QDialog::Accepted) {
        remote_name = remoteNameDialog.getRemoteName();
    } else {
        return;
    }

    if(check_lg2(git_remote_lookup(&remote, repo, strdup(remote_name.toStdString().c_str())), "Error Remote Lookup", nullptr)) {
        return;
    }

    fetch_opts.callbacks.update_tips = &update_cb;
    fetch_opts.callbacks.sideband_progress = &progress_cb;
    fetch_opts.callbacks.transfer_progress = transfer_progress_cb;
    fetch_opts.callbacks.credentials = cred_acquire_cb;

    if(check_lg2(git_remote_fetch(remote, NULL, &fetch_opts, "pull"), "Error Fetching Remote", nullptr)) {
        git_remote_free(remote);
        return;
    }

    stats = git_remote_stats(remote);

    if (stats->local_objects > 0) {
        qDebug() << "\rReceived " << stats->indexed_objects << "/" << stats->total_objects << " objects in "
                 << stats->received_bytes << " bytes (used " << stats->local_objects << " local objects" << Qt::endl;
    } else{
        qDebug() << "\rReceived " << stats->indexed_objects << "/"
                 << stats->total_objects << " objects in " << stats->received_bytes << "bytes\n";
    }

    // git_remote_free(remote);

    // -- Fetching Done --
    git_reference* origin_master = nullptr;
    git_reference* local_master = nullptr;

    int error = git_branch_lookup(&origin_master, repo, "origin/master", GIT_BRANCH_REMOTE);
    error = git_branch_lookup(&local_master, repo, "master", GIT_BRANCH_LOCAL);

    const git_annotated_commit* their_head[10];
    git_merge_options merge_opt = GIT_MERGE_OPTIONS_INIT;
    git_checkout_options checkout_opt = GIT_CHECKOUT_OPTIONS_INIT;

    error = git_repository_set_head(repo, git_reference_name(local_master));
    git_annotated_commit_from_ref((git_annotated_commit **)&their_head[0], repo, origin_master);
    git_merge(repo, their_head, 1, &merge_opt, &checkout_opt);

    if (error < 0)
    {
        const git_error *e = giterr_last();
        qCritical() << error << " / " << e->klass << " : " << e->message;

        return;
    }

    git_index* index = nullptr;

    git_repository_index(&index, repo);

    if (git_index_has_conflicts(index))
    {
        const git_index_entry* ancestor_out = nullptr;
        const git_index_entry* our_out = nullptr;
        const git_index_entry* their_out = nullptr;
        git_index_conflict_iterator* conflict_iterator = nullptr;

        git_index_conflict_iterator_new(&conflict_iterator, index);

        while (git_index_conflict_next(&ancestor_out, &our_out, &their_out, conflict_iterator) != GIT_ITEROVER)
        {
            if (ancestor_out) qWarning() << "ancestor: " << ancestor_out->path;
            if (our_out) qWarning() << "our: " << our_out->path;
            if (their_out) qWarning() << "their: " << their_out->path;
        }

        // git checkout --theirs <file>
        git_checkout_options opt = GIT_CHECKOUT_OPTIONS_INIT;
        opt.checkout_strategy |= GIT_CHECKOUT_USE_THEIRS;
        git_checkout_index(repo, index, &opt);

        git_index_conflict_iterator_free(conflict_iterator);
    }

    git_commit* their_commit = nullptr;
    git_commit* our_commit = nullptr;

    git_commit_lookup(&their_commit, repo, git_reference_target(origin_master));
    git_commit_lookup(&our_commit, repo, git_reference_target(local_master));

    // add and commit
    git_index_update_all(index, nullptr, nullptr, nullptr);
    git_index_write(index);

    git_oid new_tree_id;
    git_tree* new_tree = nullptr;
    git_signature* author = nullptr;
    git_signature* committer = nullptr;
    git_oid commit_id;

    git_index_write_tree(&new_tree_id, index);
    git_tree_lookup(&new_tree, repo, &new_tree_id);

    CommitDialog dialog = CommitDialog(this, "Author", false);
    QString author_name;
    QString author_email;
    if (dialog.exec() == QDialog::Accepted) {

        author_name = dialog.getUsername();
        author_email = dialog.getEmail();
    }


    CommitDialog dialog2 = CommitDialog(this, "Committer", false);

    QString committer_name;
    QString committer_email;
    if (dialog2.exec() == QDialog::Accepted) {

        committer_name = dialog2.getUsername();
        committer_email = dialog2.getEmail();
    }

    git_signature_now(&author, strdup(author_name.toStdString().c_str()), strdup(author_email.toStdString().c_str()));
    git_signature_now(&committer, strdup(committer_name.toStdString().c_str()), strdup(committer_email.toStdString().c_str()));

    git_commit_create_v(&commit_id, repo, git_reference_name(local_master), author, committer, "UTF-8", "pull commit", new_tree, 2, our_commit, their_commit);


    // git_oid branchOidToMerge;
    // git_repository_fetchhead_foreach(repo, fetchhead_cb, &branchOidToMerge);
    // git_annotated_commit *their_heads[1];

    // check_lg2(git_annotated_commit_lookup(&their_heads[0], repo, &branchOidToMerge), "Annotated commit lookup", nullptr);

    // git_merge_analysis_t anout;
    // git_merge_preference_t pout;

    qDebug() << "Try analysis";

    // check_lg2(git_merge_analysis(&anout, &pout, repo, (const git_annotated_commit **) their_heads, 1), "Merge analysis", nullptr);

    // if (anout & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
    //     qDebug() << "up to date";
    //     git_annotated_commit_free(their_heads[0]);
    //     git_remote_free(remote);
    //     return;
    // } else if (anout & GIT_MERGE_ANALYSIS_FASTFORWARD) {
    //     qDebug() << "fast-forwarding";

    //     git_reference *ref;
    //     git_reference *newref;
    //     git_reference *head = NULL;
    //     git_repository_head(&head, repo);
    //     const char *name = strdup(QString("refs/heads/").append(git_reference_shorthand(head)).toLocal8Bit().data());

    //     if (git_reference_lookup(&ref, repo, name) == 0)
    //         git_reference_set_target(&newref, ref, &branchOidToMerge, "pull: Fast-forward");

    //     git_reset_from_annotated(repo, their_heads[0], GIT_RESET_HARD, NULL);

    //     git_reference_free(ref);
    // }
    // git_annotated_commit_free(their_heads[0]);
    // git_remote_free(remote);
    qDebug() << "Pull Ok";
    // struct merge_options opts;
    // git_index *index;
    // git_repository_state_t state = GIT_REPOSITORY_STATE_NONE;
    // git_merge_analysis_t analysis;
    // git_merge_preference_t preference;
    // int err = 0;

    // memset(&opts, 0, sizeof(opts));

    // opts.heads = NULL;
    // opts.heads_count = 0;
    // opts.annotated = NULL;
    // opts.annotated_count = 0;

    // git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
    // git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

    // Set up merge options
    // merge_opts.flags = 0;
    // merge_opts.file_flags = GIT_MERGE_FILE_STYLE_DIFF3;

    // // Set up checkout options
    // checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    // Perform the merge
    // if (check_lg2(git_merge(repo, NULL, 0, &merge_opts, &checkout_opts), "merge failed", NULL)) {
    //     git_remote_free(remote);
    //     // free((char **)opts.heads);
    //     // free(opts.annotated);
    // }

    // git_remote_free(remote);
    git_repository_state_cleanup(repo);
}



Git::~Git() {

    git_repository_free(repo);
    git_libgit2_shutdown();
}


bool Git::doesRepositoryExist(const char* path) {
    int error = git_repository_open(&repo, path);
    if (error == 0) {
        // Repository exists
        git_repository_free(repo);
        return true;
    }
    return false;
}

//callback fucntion for printing added files
int print_matched_cb(const char *path, const char *matched_pathspec, void *payload)
{
    struct index_options *opts = (struct index_options *)(payload);
    int ret;
    unsigned status;
    (void)matched_pathspec;

    /* Get the file status */
    if (git_status_file(&status, opts->repo, path) < 0)
        return -1;

    if ((status & GIT_STATUS_WT_MODIFIED) || (status & GIT_STATUS_WT_NEW)) {
        // printf("add '%s'\n", path);
        ret = 0;
    } else {
        ret = 1;
    }

    if (opts->dry_run)
        ret = 1;

    return ret;
}

bool check_lg2(int error, const char *message, const char *extra)
{
    const git_error *lg2err;
    const char *lg2msg = "", *lg2spacer = "";

    if (!error)
        return false;

    if ((lg2err = git_error_last()) != NULL && lg2err->message != NULL) {
        lg2msg = lg2err->message;
        lg2spacer = " - ";
    }

    if (extra)
        qCritical() << message << " " << extra << " ["  << error << "] " <<  lg2spacer << " "  << lg2msg;
    else
        qCritical() << message << "[" << error << "]" << lg2spacer << lg2msg;

    return true;
}

static int readline(char **out)
{
    int c, error = 0, length = 0, allocated = 0;
    char *line = NULL;

    errno = 0;

    while ((c = getchar()) != EOF) {
        if (length == allocated) {
            allocated += 16;

            if ((line = (char*)realloc(line, allocated)) == nullptr) {
                error = -1;
                goto error;
            }
        }

        if (c == '\n')
            break;

        line[length++] = c;
    }

    if (errno != 0) {
        error = -1;
        goto error;
    }

    line[length] = '\0';
    *out = line;
    line = NULL;
    error = length;
error:
    free(line);
    return error;
}

// static int ask(char **out, const char *prompt, char optional)
// {
//     printf("%s ", prompt);
//     fflush(stdout);

//     // if (!readline(out) && !optional) {
//     //     fprintf(stderr, "Could not read response: %s", strerror(errno));
//     //     return -1;
//     // }

//     CredentialsDialog dialog = new CredentialsDialog();
//     if (dialog.exec() == QDialog::Accepted) {
//         QString username = dialog.getUsername();
//         *out = strdup(username.toUtf8().constData());
//         // return git_cred_userpass_plaintext_new(out, username.toStdString().c_str(), password.toStdString().c_str());
//         if (*out == NULL && !optional) {
//             qCritical("Failed to allocate memory for username\n");
//             return -1;
//         }

//         return 0;
//     }

//     if (!optional) {
//         fprintf(stderr, "Username input canceled\n");
//         return -1;
//     }

//     return 0;
// }

static int ask(char **out, const char *prompt, char optional, bool secure = false)
{

    CredentialsDialog dialog = CredentialsDialog();
    if(secure)
        dialog.setPrompt(prompt, true);
    else
        dialog.setPrompt(prompt, false);

    // dialog.exec();

    if (dialog.exec() == QDialog::Accepted) {
        QString data = dialog.getData();

        *out = strdup(data.toUtf8().constData());
        if (*out == NULL && !optional) {
            qCritical("Failed to allocate memory for username\n");
            return -1;
        }

        return 0;
    }

    if (!optional) {
        qCritical("Username input canceled\n");
        return -1;
    }

    return 0;
}


int cred_acquire_cb(git_credential **out,
                    const char *url,
                    const char *username_from_url,
                    unsigned int allowed_types,
                    void *payload)
{
    char *username = NULL, *password = NULL, *privkey = NULL, *pubkey = NULL;
    int error = 1;

    UNUSED(url);
    UNUSED(payload);

    if (username_from_url) {
        if ((username = strdup(username_from_url)) == NULL)
            goto out;
    } else if ((error = ask(&username, "Username:", 0)) < 0) {
        goto out;
    }

    if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
        int n;

        if ((error = ask(&privkey, "SSH Key:", 0)) < 0 ||
            (error = ask(&password, "Token/Password:", 1, true)) < 0)
            goto out;

        if ((n = snprintf(NULL, 0, "%s.pub", privkey)) < 0 ||
            (pubkey = (char*)malloc(n + 1)) == NULL ||
            (n = snprintf(pubkey, n + 1, "%s.pub", privkey)) < 0)
            goto out;

        error = git_credential_ssh_key_new(out, username, pubkey, privkey, password);
    } else if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        if ((error = ask(&password, "Token/Password:", 1, true)) < 0)
            goto out;

        error = git_credential_userpass_plaintext_new(out, username, password);
    } else if (allowed_types & GIT_CREDENTIAL_USERNAME) {
        error = git_credential_username_new(out, username);
    }

out:
    free(username);
    free(password);
    free(privkey);
    free(pubkey);
    return error;
}

bool match_arg(int *out, struct args_info *args, const char *opt)
{
    const char *found = args->argv[args->pos];

    if (!strcmp(found, opt)) {
        *out = 1;
        return 1;
    }

    if (!strncmp(found, "--no-", strlen("--no-")) &&
        !strcmp(found + strlen("--no-"), opt + 2)) {
        *out = 0;
        return 1;
    }

    *out = -1;
    return 0;
}

// Custom callback to handle authentication
int credentialsCallback(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload) {
    Q_UNUSED(url);
    Q_UNUSED(username_from_url);
    Q_UNUSED(allowed_types);
    Q_UNUSED(payload);
    git_repository *repo = static_cast<git_repository*>(payload);
    QCoreApplication::processEvents();

    /*CredentialsDialog dialog = new CredentialsDialog();
    if (dialog.exec() == QDialog::Accepted) {
        QString username = dialog.getUsername();
        // QString password = dialog.getPassword();

        return git_cred_userpass_plaintext_new(out, username.toStdString().c_str(), password.toStdString().c_str());

    }*/ /*else {
        return GIT_PASSTHROUGH; // or another specific error code
    }*/

    return GIT_ERROR;
}

static int progress_cb(const char *str, int len, void *data)
{
    (void)data;
    QString qStr = QString(str);
    qStr.truncate(len);
    qDebug() << "remote: " << qStr;
    return 0;
}

static int update_cb(const char *refname, const git_oid *a, const git_oid *b, void *data)
{
    char a_str[GIT_OID_SHA1_HEXSIZE+1], b_str[GIT_OID_SHA1_HEXSIZE+1];
    (void)data;

    git_oid_fmt(b_str, b);
    b_str[GIT_OID_SHA1_HEXSIZE] = '\0';

    QString qSa_str = QString(a_str);
    QString qSb_str = QString(b_str);
    qSa_str.truncate(10);
    qSb_str.truncate(20);

    if (git_oid_is_zero(a)) {
        qDebug() << "[new]     " << qSb_str << refname << Qt::endl;
    } else {
        qSb_str.truncate(10);
        git_oid_fmt(a_str, a);
        a_str[GIT_OID_SHA1_HEXSIZE] = '\0';
        qDebug() << "[updated] " << qSa_str << ".." << qSb_str << " " << refname << "\n";
    }

    return 0;
}

static int transfer_progress_cb(const git_indexer_progress *stats, void *payload)
{
    (void)payload;

    if (stats->received_objects == stats->total_objects) {
        qDebug() << "Resolving deltas " << stats->indexed_deltas << "/" << stats->total_deltas << "\r";
    } else if (stats->total_objects > 0) {
        qDebug() << "Received " << stats->received_objects
                 << "/" << stats->total_objects
                 << " objects (" << stats->indexed_objects
                 << ") in " << stats->received_bytes << " bytes\r";
    }
    return 0;
}


//for merge

void *xrealloc(void *oldp, size_t newsz)
{
    void *p = realloc(oldp, newsz);
    if (p == NULL) {
        qDebug() << "Cannot allocate memory, exiting.\n";
        //crash
        exit(1);
    }
    return p;
}

static void print_usage(void)
{
    qDebug() << "usage: merge [--no-commit] <commit...>\n";
    exit(1);
}

static void merge_options_init(struct merge_options *opts)
{
    memset(opts, 0, sizeof(*opts));

    opts->heads = NULL;
    opts->heads_count = 0;
    opts->annotated = NULL;
    opts->annotated_count = 0;
}

static void opts_add_refish(struct merge_options *opts, const char *refish)
{
    size_t sz;
    // assert(opts != NULL);
    if (opts != nullptr) {
        sz = ++opts->heads_count * sizeof(opts->heads[0]);
        opts->heads = (const char**) xrealloc((void *) opts->heads, sz);
        opts->heads[opts->heads_count - 1] = refish;
    }
}

static void parse_options(const char **repo_path, struct merge_options *opts, int argc, char **argv)
{
    struct args_info args = ARGS_INFO_INIT;

    if (argc <= 1)
        print_usage();

    for (args.pos = 1; args.pos < argc; ++args.pos) {
        const char *curr = argv[args.pos];

        if (curr[0] != '-') {
            opts_add_refish(opts, curr);
        } else if (!strcmp(curr, "--no-commit")) {
            opts->no_commit = 1;
        } /*else if (match_str_arg(repo_path, &args, "--git-dir")) {
            continue;
        }*/ else {
            print_usage();
        }
    }
}

int resolve_refish(git_annotated_commit **commit, git_repository *repo, const char *refish)
{
    git_reference *ref;
    git_object *obj;
    int err = 0;

    assert(commit != NULL);

    err = git_reference_dwim(&ref, repo, refish);
    if (err == GIT_OK) {
        git_annotated_commit_from_ref(commit, repo, ref);
        git_reference_free(ref);
        return 0;
    }

    err = git_revparse_single(&obj, repo, refish);
    if (err == GIT_OK) {
        err = git_annotated_commit_lookup(commit, repo, git_object_id(obj));
        git_object_free(obj);
    }

    return err;
}

static int resolve_heads(git_repository *repo, struct merge_options *opts)
{
    git_annotated_commit **annotated = (git_annotated_commit**) calloc(opts->heads_count, sizeof(git_annotated_commit *));
    size_t annotated_count = 0, i;
    int err = 0;

    for (i = 0; i < opts->heads_count; i++) {
        err = resolve_refish(&annotated[annotated_count++], repo, opts->heads[i]);
        if (err != 0) {
            fprintf(stderr, "failed to resolve refish %s: %s\n", opts->heads[i], git_error_last()->message);
            annotated_count--;
            continue;
        }
    }

    if (annotated_count != opts->heads_count) {
        fprintf(stderr, "unable to parse some refish\n");
        free(annotated);
        return -1;
    }

    opts->annotated = annotated;
    opts->annotated_count = annotated_count;
    return 0;
}

static int perform_fastforward(git_repository *repo, const git_oid *target_oid, int is_unborn)
{
    git_checkout_options ff_checkout_options = GIT_CHECKOUT_OPTIONS_INIT;
    git_reference *target_ref;
    git_reference *new_target_ref;
    git_object *target = NULL;
    int err = 0;

    if (is_unborn) {
        const char *symbolic_ref;
        git_reference *head_ref;

        /* HEAD reference is unborn, lookup manually so we don't try to resolve it */
        err = git_reference_lookup(&head_ref, repo, "HEAD");
        if (err != 0) {
            fprintf(stderr, "failed to lookup HEAD ref\n");
            return -1;
        }

        /* Grab the reference HEAD should be pointing to */
        symbolic_ref = git_reference_symbolic_target(head_ref);

        /* Create our master reference on the target OID */
        err = git_reference_create(&target_ref, repo, symbolic_ref, target_oid, 0, NULL);
        if (err != 0) {
            fprintf(stderr, "failed to create master reference\n");
            return -1;
        }

        git_reference_free(head_ref);
    } else {
        /* HEAD exists, just lookup and resolve */
        err = git_repository_head(&target_ref, repo);
        if (err != 0) {
            fprintf(stderr, "failed to get HEAD reference\n");
            return -1;
        }
    }

    /* Lookup the target object */
    err = git_object_lookup(&target, repo, target_oid, GIT_OBJECT_COMMIT);
    if (err != 0) {
        fprintf(stderr, "failed to lookup OID %s\n", git_oid_tostr_s(target_oid));
        return -1;
    }

    /* Checkout the result so the workdir is in the expected state */
    ff_checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;
    err = git_checkout_tree(repo, target, &ff_checkout_options);
    if (err != 0) {
        fprintf(stderr, "failed to checkout HEAD reference\n");
        return -1;
    }

    /* Move the target reference to the target OID */
    err = git_reference_set_target(&new_target_ref, target_ref, target_oid, NULL);
    if (err != 0) {
        fprintf(stderr, "failed to move HEAD reference\n");
        return -1;
    }

    git_reference_free(target_ref);
    git_reference_free(new_target_ref);
    git_object_free(target);

    return 0;
}

static void output_conflicts(git_index *index)
{
    git_index_conflict_iterator *conflicts;
    const git_index_entry *ancestor;
    const git_index_entry *our;
    const git_index_entry *their;
    int err = 0;

    check_lg2(git_index_conflict_iterator_new(&conflicts, index), "failed to create conflict iterator", NULL);

    while ((err = git_index_conflict_next(&ancestor, &our, &their, conflicts)) == 0) {
        fprintf(stderr, "conflict: a:%s o:%s t:%s\n",
                ancestor ? ancestor->path : "NULL",
                our->path ? our->path : "NULL",
                their->path ? their->path : "NULL");
    }

    if (err != GIT_ITEROVER) {
        fprintf(stderr, "error iterating conflicts\n");
    }

    git_index_conflict_iterator_free(conflicts);
}

static int create_merge_commit(git_repository *repo, git_index *index, struct merge_options *opts)
{
    git_oid tree_oid, commit_oid;
    git_tree *tree = NULL;
    git_signature *sign = NULL;
    git_reference *merge_ref = NULL;
    git_annotated_commit *merge_commit;
    git_reference *head_ref = NULL;
    git_commit **parents = (git_commit**) calloc(opts->annotated_count + 1, sizeof(git_commit *));
    const char *msg_target = NULL;
    size_t msglen = 0;
    char *msg = NULL;
    size_t i;
    int err;

    /* Grab our needed references */
    check_lg2(git_repository_head(&head_ref, repo), "failed to get repo HEAD", NULL);
    if (resolve_refish(&merge_commit, repo, opts->heads[0])) {
        fprintf(stderr, "failed to resolve refish %s", opts->heads[0]);
        free(parents);
        return -1;
    }

    /* Maybe that's a ref, so DWIM it */
    err = git_reference_dwim(&merge_ref, repo, opts->heads[0]);
    check_lg2(err, "failed to DWIM reference", git_error_last()->message);

    /* Grab a signature */
    check_lg2(git_signature_now(&sign, "Me", "me@example.com"), "failed to create signature", NULL);

#define MERGE_COMMIT_MSG "Merge %s '%s'"
    /* Prepare a standard merge commit message */
    if (merge_ref != NULL) {
        check_lg2(git_branch_name(&msg_target, merge_ref), "failed to get branch name of merged ref", NULL);
    } else {
        msg_target = git_oid_tostr_s(git_annotated_commit_id(merge_commit));
    }

    msglen = snprintf(NULL, 0, MERGE_COMMIT_MSG, (merge_ref ? "branch" : "commit"), msg_target);
    if (msglen > 0) msglen++;
    msg = (char*) malloc(msglen);
    err = snprintf(msg, msglen, MERGE_COMMIT_MSG, (merge_ref ? "branch" : "commit"), msg_target);

    /* This is only to silence the compiler */
    if (err < 0) goto cleanup;

    /* Setup our parent commits */
    err = git_reference_peel((git_object **)&parents[0], head_ref, GIT_OBJECT_COMMIT);
    check_lg2(err, "failed to peel head reference", NULL);
    for (i = 0; i < opts->annotated_count; i++) {
        git_commit_lookup(&parents[i + 1], repo, git_annotated_commit_id(opts->annotated[i]));
    }

    /* Prepare our commit tree */
    check_lg2(git_index_write_tree(&tree_oid, index), "failed to write merged tree", NULL);
    check_lg2(git_tree_lookup(&tree, repo, &tree_oid), "failed to lookup tree", NULL);

    /* Commit time ! */
    err = git_commit_create_v(&commit_oid,
                            repo, git_reference_name(head_ref),
                            sign, sign,
                            NULL, msg,
                            tree,
                            opts->annotated_count + 1, (const git_commit**)parents);
    check_lg2(err, "failed to create commit", NULL);

    /* We're done merging, cleanup the repository state */
    git_repository_state_cleanup(repo);

cleanup:
    // free(parents);
    // return err;
    if (msg) free(msg);
    if (sign) git_signature_free(sign);
    if (tree) git_tree_free(tree);
    if (merge_commit) git_annotated_commit_free(merge_commit);
    if (merge_ref) git_reference_free(merge_ref);
    if (head_ref) git_reference_free(head_ref);
    if (parents) {
        for (i = 0; i < opts->annotated_count + 1; i++) {
            if (parents[i]) git_commit_free(parents[i]);
        }
        free(parents);
    }
    return err;
}

// int fetchhead_cb(const char *ref_name, const char *remote_url, const git_oid *oid, unsigned int is_merge, void *payload)
// {
//     if (is_merge)
//     {
//         printf("reference: '%s' is the reference we should merge\n");
//         git_oid_cpy((git_oid *)payload, oid);
//     }
// }
