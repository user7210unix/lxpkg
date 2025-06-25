import configparser
import sys

def parse_config(config_file):
    configparser = configparser.ConfigParser()
    config.read(config_file)
    result = {}
    result['db_path'] = config.get('General', 'db_path', fallback='/var/db/lxpkg')
    result['repo_path'] = config.get('General', 'repo_path', fallback='/usr/portage')
    result['repo_subdirs'] = config.get('General', 'repo_subdirs', fallback='core,wayland,extra,community')
    result['make_opts'] = config.get('General', 'make_opts', fallback='-j$(nproc)')
    result['use_flags'] = config.get('General', 'use_flags', fallback='gtk,qt')
    result['max_jobs'] = config.get('General', 'max_jobs', fallback='$(nproc)')
    result['repos'] = config.get('General', 'repos', fallback='https://github.com/LearnixOS/repo')
    result['cache_dir'] = config.get('General', 'cache_dir', fallback='/var/cache/lxpkg')
    return result

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)
        config = parse_config(sys.argv[1])
        for key, value in config.items():
            print(f"{key}={value}")