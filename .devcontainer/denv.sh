#!/usr/bin/env bash
cd "$( dirname "$0" )"
pushd .. > /dev/null
rel=$(grep FROM .devcontainer/centos7-bld.dockerfile)
dkr=$(echo "${rel}" | cut -d" " -f2) # ghcr.io/smanders/buildpro/centos7-bld:TAG
hst=$(echo "${dkr}" | cut -d/ -f1) # ghcr.io
rel=$(echo "${rel}" | cut -d- -f2) # bld:TAG
rel=${rel//:}
rel=bp${rel/./-}
display_host=$(echo ${DISPLAY} | cut -d: -f1)
if [[ -z "${display_host}" ]]; then
  display_env=${DISPLAY}
  xauth_env=
elif [[ "${display_host}" == "localhost" ]]; then
  echo "NOTE: X11UseLocalhost should be no in /etc/ssh/sshd_config"
else
  display_screen=$(echo $DISPLAY | cut -d: -f2)
  display_num=$(echo ${display_screen} | cut -d. -f1)
  magic_cookie=$(xauth list ${DISPLAY} | awk '{print $3}')
  xauth_file=/tmp/.X11-unix/docker.xauth
  docker_host=$(ip -4 addr show docker0 | grep -Po 'inet \K[\d.]+')
  touch ${xauth_file}
  xauth -f ${xauth_file} add ${docker_host}:${display_num} . ${magic_cookie}
  display_env=${docker_host}:${display_screen}
  xauth_env=${xauth_file}
fi
env="COMPOSE_PROJECT_NAME=${PWD##*/}"
env="${env}\nHNAME=${rel}"
env="${env}\nUSERID=$(id -u ${USER})"
env="${env}\nGROUPID=$(id -g ${USER})"
env="${env}\nTZ=$(head -n 1 /etc/timezone)"
env="${env}\nDISPLAY_ENV=${display_env}"
env="${env}\nXAUTH_ENV=${xauth_env}"
cr8="#/usr/bin/env bash"
cr8="${cr8}\ncd \"\$( dirname \"\$0\" )\""
cr8="${cr8}\ndocker pull ${dkr}"
cr8="${cr8}\necho \"saving docker image ${dkr}...\""
cr8="${cr8}\ndocker save ${dkr} | pv -s \$(docker image inspect ${dkr} --format='{{.Size}}') | bzip2 > docker.${rel}.tar.bz2"
##############################
isrhub=isrhub.usurf.usu.edu
if command -v host >/dev/null && host ${isrhub} | grep "has address" >/dev/null; then
  urlPfx="https://${isrhub}"
  doisrhub=true
  ver="docker image built online"
  env="${env}\nADDSRC1=.env"
  env="${env}\nADDSRC2=.env"
else
  doisrhub=false
  ver="docker image built offline"
  env="${env}\nADDSRC1=_bld/*.tar.xz"
  env="${env}\nADDSRC2=_bld/*.sh"
fi
##############################
# NOTE: EXTERN_DIR and GCC_VER need to match buildpro/image/centos7-pro.dockerfile
EXTERN_DIR=/opt/extern
GCC_VER=gcc731
##############################
function findVer
{
  local val=$1
  shift
  for loc in "$@"; do
    if [ -f $loc ]; then
      local gver=`grep "$val" $loc`
      [[ ! -z "$gver" ]] && break
    fi
  done
  local fver=`echo ${gver} | awk '{$1=$1};1' | cut -d " " -f2 | cut -d ")" -f1`
  echo "$fver"
}
##############################
wproVer="$(findVer 'set(webpro_REV' CMakeLists.txt */CMakeLists.txt */defaults.txt)"
if [[ -n "${wproVer}" ]]; then
  wproBase=webpro-${wproVer}-${GCC_VER}-64-Linux
  if [[ ${wproVer} < "20.05.1" ]]; then
    if ${doisrhub}; then
      WEBPRO_DL="wget -q \"${urlPfx}/webpro/webpro/releases/download/${wproVer}/${wproBase}.sh\" \
&& chmod 755 webpro*.sh "
    else
      WEBPRO_DL="cd ${EXTERN_DIR}"
    fi
    WEBPRO="${WEBPRO_DL} \
&& ./${wproBase}.sh --prefix=${EXTERN_DIR} --include-subdir \
&& rm ${wproBase}.sh"
  elif ${doisrhub}; then
    WEBPRO_DL="wget ${urlPfx}/webpro/webpro/releases/download/${wproVer}/${wproBase}.tar.xz"
    WEBPRO="${WEBPRO_DL} -qO- | tar -xJ -C ${EXTERN_DIR}"
  fi
fi
env="${env}\nWEBPRO=${WEBPRO}"
[[ -n ${WEBPRO_DL} ]] && cr8="${cr8}\n${WEBPRO_DL}"
##############################
iproVer="$(findVer 'set(internpro_REV' CMakeLists.txt */toplevel.cmake */*/toplevel.cmake */defaults.txt)"
if [[ -n "${iproVer}" ]] && ${doisrhub}; then
  INTERNPRO_DL="wget ${urlPfx}/smanders/internpro/releases/download/${iproVer}/internpro-${iproVer}-${GCC_VER}-64-Linux.tar.xz"
  INTERNPRO="${INTERNPRO_DL} -qO- | tar -xJ -C ${EXTERN_DIR}"
fi
env="${env}\nINTERNPRO=${INTERNPRO}"
[[ -n ${INTERNPRO_DL} ]] && cr8="${cr8}\n${INTERNPRO_DL}"
##############################
psdkVer="$(findVer 'PluginSDK_REV' CMakeLists.txt */defaults.txt)"
if [[ ${psdkVer} == "v3.0.3.0" ]]; then
  pfx=Vantage
else
  pfx=SDL
fi
if [[ -n "${psdkVer}" ]] && ${doisrhub}; then
  PLUGINSDK_DL="wget ${urlPfx}/PluginFramework/SDKSuper/releases/download/${psdkVer}/${pfx}PluginSDK-${psdkVer}-${GCC_VER}-64-Linux.tar.xz"
  PLUGINSDK="${PLUGINSDK_DL} -qO- | tar -xJ -C ${EXTERN_DIR}"
fi
env="${env}\nPLUGINSDK=${PLUGINSDK}"
[[ -n ${PLUGINSDK_DL} ]] && cr8="${cr8}\n${PLUGINSDK_DL}"
##############################
if [ -f .crtoolrc ]; then
  crtv=`grep version .crtoolrc`
elif [ -f image/defaults.txt ]; then
  crtv=`grep CRTOOL_REV image/defaults.txt`
fi
crToolVer=`echo ${crtv} | awk '{$1=$1};1' | cut -d " " -f2 | cut -d "\"" -f2`
crWrapVer=20.07.1
if [[ -n "${crToolVer}" && -n "${crWrapVer}" ]]; then
  if ${doisrhub}; then
    CRTOOL_DL="wget -q \"${urlPfx}/CRTool/CRTool/releases/download/${crWrapVer}/CRTool-${crWrapVer}.sh\" \
&& wget -q \"${urlPfx}/CRTool/CRToolImpl/releases/download/${crToolVer}/CRToolImpl-${crToolVer}.sh\" \
&& chmod 755 CRTool*.sh"
  else
    CRTOOL_DL="cd ${EXTERN_DIR}"
  fi
  CRTOOL="mkdir ${EXTERN_DIR}/CRTool \
&& ${CRTOOL_DL} \
&& ./CRTool-${crWrapVer}.sh --prefix=${EXTERN_DIR}/CRTool --exclude-subdir \
&& ./CRToolImpl-${crToolVer}.sh --prefix=${EXTERN_DIR} --include-subdir \
&& rm CRTool-${crWrapVer}.sh \
&& rm CRToolImpl-${crToolVer}.sh"
fi
env="${env}\nCRTOOL=${CRTOOL}"
[[ -n ${CRTOOL_DL} ]] && cr8="${cr8}\n${CRTOOL_DL}"
##############################
echo -e "${env}" > .env
[[ -n ${ver} ]] && echo -e "${ver}" > .devcontainer/.env
##############################
offlineDir=.devcontainer/_bld
if command -v host >/dev/null && host ${hst} | grep "not found" >/dev/null; then
  if ! docker inspect ${dkr} > /dev/null 2>&1; then
    if [[ -f ${offlineDir}/docker.${rel}.tar.bz2 ]]; then
      # if the host specified by FROM isn't reachable, the docker image isn't local, and the offline tar.bz2 exists
      # then load the offline docker image
      echo "loading docker image from docker.${rel}.tar.bz2..."
      pv ${offlineDir}/docker.${rel}.tar.bz2 | sudo docker load
    fi
  fi
fi
if [ -d ${offlineDir} ]; then
  if ${doisrhub}; then
    if [[ -x ${offlineDir}/create.bash ]]; then
      # if the offlineDir exists, isrhub is reachable, the create script exists,
      # then wipeout the offlineDir because it shouldn't be in the build context
      rm -rf ${offlineDir}
    else
      # if the offlineDir exists, isrhub is reachable, the create script doesn't exist,
      # then create the offline container bundle (docker-compose.sh -c)
      echo -e "${cr8}" > ${offlineDir}/create.bash
      chmod 755 ${offlineDir}/create.bash
      ./${offlineDir}/create.bash
      ls -l ${offlineDir}
      du -sh ${offlineDir}
    fi
  elif [[ ! -x ${offlineDir}/create.bash ]]; then
    # if the offlineDir exists, but the create script doesn't, and isrhub isn't reachable,
    # then the offline container bundle can't be created (docker-compose.sh -c)
    rm -rf ${offlineDir}
    echo -e "ERROR: can't create offline container bundle when ${isrhub} not accessible"
  fi
elif ! ${doisrhub}; then
  # the offlineDir doesn't exist, and isrhub isn't reachable
  echo "NOTE: create offline container bundle with 'docker-compose.sh -c'"
fi
popd > /dev/null
