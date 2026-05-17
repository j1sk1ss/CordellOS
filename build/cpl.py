import os
import subprocess

from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Environment import Environment


def _split(env: Environment, value):
    if value is None:
        return []
    return [env.subst(str(item)) for item in env.Split(value) if env.subst(str(item))]


def _flatten(value):
    if value is None:
        return []
    if isinstance(value, (list, tuple)):
        result = []
        for item in value:
            result.extend(_flatten(item))
        return result
    return [value]


def _append_option(args, option, value):
    if option and value:
        args.extend([option, value])


def _include_flags(env: Environment):
    prefix = env.subst('$CPLINCPREFIX')
    suffix = env.subst('$CPLINCSUFFIX')
    includes = []
    seen = set()

    for include_dir in _split(env, env.get('CPLPATH', [])) + _split(env, env.get('CPPPATH', [])):
        if include_dir in seen:
            continue
        seen.add(include_dir)
        includes.append(f'{prefix}{include_dir}{suffix}')

    return includes


def _tool_flags(env: Environment):
    args = []

    _append_option(args, env.subst('$CPLASOPTION'), env.subst('$AS'))
    _append_option(args, env.subst('$CPLASMFORMATOPTION'), env.subst('$CPLASMFORMAT'))
    _append_option(args, env.subst('$CPLLDOPTION'), env.subst('$LD'))
    _append_option(args, env.subst('$CPLCODESECTIONOPTION'), env.subst('$CPLCODESECTION'))
    _append_option(args, env.subst('$CPLROSECTIONOPTION'), env.subst('$CPLROSECTION'))
    _append_option(args, env.subst('$CPLGLOBSECTIONOPTION'), env.subst('$CPLGLOBSECTION'))
    args.extend(_split(env, env.get('CPLLINKFLAGS', [])))

    return args


def _cpl_command(
    env: Environment,
    mode_flag,
    target,
    source,
    output_flag='$CPLOUTPUTFLAG',
    use_include_flags=True,
    use_tool_flags=True,
):
    cpl = env.subst('$CPL')
    if not os.path.isabs(cpl) and os.path.exists(cpl):
        cpl = os.path.abspath(cpl)

    args = [cpl]
    args.extend(_split(env, env.get('CPLFLAGS', [])))
    if use_include_flags:
        args.extend(_include_flags(env))
    if use_tool_flags:
        args.extend(_tool_flags(env))

    mode = env.subst(mode_flag)
    if mode:
        args.extend(_split(env, mode))

    output = env.subst(output_flag)
    if output:
        args.extend([output, target[0].abspath])
    else:
        args.append(target[0].abspath)

    args.extend(src.srcnode().abspath for src in source)
    return args


def _cpl_emit_asm_action(target, source, env):
    target_path = target[0].abspath
    target_dir = os.path.dirname(target_path)
    if target_dir:
        os.makedirs(target_dir, exist_ok=True)

    command = _cpl_command(
        env,
        '$CPLEMITASMFLAGS',
        target,
        source,
        output_flag='$CPLASMOUTPUTFLAG',
        use_include_flags=False,
        use_tool_flags=True,
    )
    subprocess.check_call(command)
    if not os.path.exists(target_path):
        raise RuntimeError(f'CPL compiler did not produce {target_path}')

    return 0


def _cpl_asm_generator(target, source, env, for_signature):
    return Action(_cpl_emit_asm_action, '$CPLASMCOMSTR')


def _cpl_object_generator(target, source, env, for_signature):
    return Action(_cpl_command(env, '$CPLCOMPILEFLAG', target, source), '$CPLOBJCOMSTR')


def _cpl_program_generator(target, source, env, for_signature):
    return Action(_cpl_command(env, '$CPLLINKFLAG', target, source), '$CPLLINKCOMSTR')


def _cpl_target_name(env: Environment, src, suffix):
    src_node = env.File(src).srcnode()
    src_root = env.Dir('.').srcnode().abspath
    src_path = os.path.relpath(src_node.abspath, src_root)
    base, _ = os.path.splitext(src_path)
    return f'{base}_cpl{suffix}'


def _cpl_object(env: Environment, source):
    sources = _flatten(source)
    if env.subst('$CPL_OBJECT_MODE') == 'object':
        objects = []
        for src in sources:
            objects.extend(_flatten(env.CPLDirectObject(_cpl_target_name(env, src, '.o'), src)))
        return objects

    objects = []
    for src in sources:
        asm_sources = env.CPLAsm(_cpl_target_name(env, src, '.asm'), src)
        objects.extend(_flatten(env.Object(_cpl_target_name(env, src, '.o'), asm_sources)))
    return objects


def setup_cpl_builders(env: Environment):
    env.SetDefault(
        CPL='cpl',
        CPLFLAGS=[],
        CPLPATH=[],
        CPLLINKFLAGS=[],
        CPL_OBJECT_MODE='asm',
        CPLEMITASMFLAGS=['--emit-asm', '--no-compile'],
        CPLCOMPILEFLAG='',
        CPLLINKFLAG='',
        CPLOUTPUTFLAG='--output',
        CPLASMOUTPUTFLAG='--asm-output',
        CPLINCPREFIX='-I',
        CPLINCSUFFIX='',
        CPLASOPTION='--asm-compiler',
        CPLASMFORMAT='elf32',
        CPLASMFORMATOPTION='--asm-format',
        CPLLDOPTION='--linker',
        CPLCODESECTION='.text',
        CPLCODESECTIONOPTION='--code-section',
        CPLROSECTION='.rodata',
        CPLROSECTIONOPTION='--ro-section',
        CPLGLOBSECTION='.data',
        CPLGLOBSECTIONOPTION='--glob-section',
        CPLASMCOMSTR='CPL -> asm [$SOURCE]',
        CPLOBJCOMSTR='CPL -> obj [$SOURCE]',
        CPLLINKCOMSTR='CPL linking [$TARGET]',
    )

    env.Append(
        BUILDERS={
            'CPLAsm': Builder(
                generator=_cpl_asm_generator,
                suffix='.asm',
                src_suffix='.cpl',
            ),
            'CPLDirectObject': Builder(
                generator=_cpl_object_generator,
                suffix='.o',
                src_suffix='.cpl',
            ),
            'CPLProgram': Builder(
                generator=_cpl_program_generator,
                suffix='.elf',
                src_suffix='.cpl',
            ),
        }
    )

    env.AddMethod(_cpl_object, 'CPLObject')
