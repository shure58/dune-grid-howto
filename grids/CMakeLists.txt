set(GRIDS
  2dgrid.al
  2dsimplex.alu
  3dgrid.al
  unitcube1.dgf
  unitcube2.dgf
  unitcube3.dgf)

install(FILES ${GRIDS} DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples/grids)
