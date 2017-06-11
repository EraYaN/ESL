c = {'Naive', 'Optimised', 'Optimised (fixp.)', 'NEON (compiler only)', 'NEON (compiler only, fixp.)', 'NEON', 'NEON (fixp.)', 'DSP', 'DSP + Optimised', 'DSP + NEON'};
speedups = [1 1; 20.3 4.8; 31.8 5.1; 20.3 4.8; 31.9 5.1; 25.2 4.9; 48.1 5.3; 11.4 4.1; 14.0 4.3; 20.3 4.6];
bar(speedups)
line(xlim, [1 1], 'LineStyle', '--', 'Color', [0.2, 0.2, 0.2]);
ax = gca;
ax.XTickLabelRotation = 45;
ax.XTickLabel = c;
ylabel('Speed-up [x]')

addpath('matlab2tikz')
matlab2tikz( '../docs/assignment2/comparison/resources/comparison.tikz', 'height', '\figureheight', 'width', '\figurewidth' );
