import subprocess

Import("env")


def git_cmd(project_dir, args):
    return subprocess.check_output(
        ["git", "-C", project_dir] + args,
        text=True,
    ).strip()


def git_readable_version():
    project_dir = env.subst("$PROJECT_DIR")
    try:
        # Readable + sortable: YYYYMMDD.<commit_count>
        commit_date = git_cmd(project_dir, ["show", "-s", "--date=format:%Y%m%d", "--format=%cd", "HEAD"])
        commit_count = git_cmd(project_dir, ["rev-list", "--count", "HEAD"])
        rev = f"{commit_date}.{commit_count}"
    except Exception:
        return "dev"

    try:
        dirty = subprocess.call(
            ["git", "-C", project_dir, "diff", "--quiet", "HEAD", "--"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if dirty != 0:
            rev += "-dirty"
    except Exception:
        pass
    return rev


version = git_readable_version()
env.Append(BUILD_FLAGS=[f'-DSOFTWARE_VERSION=\\"{version}\\"'])
print(f"Firmware version: {version}")
